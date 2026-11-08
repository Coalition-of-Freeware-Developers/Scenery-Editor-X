// Labels new issues from their issue-form answers and asks the reporter for details a fix needs.
// Run by .github/workflows/issue-triage.yml through actions/github-script; configured by .github/issue-triage.json.
//
//   issues opened / edited / reopened: adds the labels the answers map to (Area, Platform, Impact, regression), adds
//     NEEDS TRIAGE to new issues from outside the team, and keeps one comment (marked MARKER) listing missing
//     details, labelled FEEDBACK. When the reporter edits the issue to fill them in, the comment says so and FEEDBACK goes.
//   issues opened with no one assigned: assigns newIssues.assignees.
//   issue_comment created by the issue's author on an issue labelled FEEDBACK: swaps FEEDBACK for NEEDS TRIAGE, so a
//     maintainer looks again.
//
// It only ever adds labels from the answers, never removes them: a maintainer's relabelling stays.
'use strict';

const { readGithubJson, ensureLabels } = require('./labels.js');

const MARKER = '<!-- sedx-issue-triage -->';
const COMPLETE_MARKER = '<!-- sedx-issue-triage:complete -->';
const NO_RESPONSE = '_No response_';
const TEAM_ASSOCIATIONS = new Set(['OWNER', 'MEMBER', 'COLLABORATOR']);
const NEEDS_TRIAGE = 'NEEDS TRIAGE';
const NEEDS_INFO = 'FEEDBACK';

/**
 * Splits an issue-form body into its answers: GitHub writes each field as `### <label>`, a blank line, then the answer
 * (`_No response_` when left empty). Headings inside fenced code blocks (pasted logs) don't start a new field.
 * @returns {Map<string, string>} label -> answer ('' when empty)
 */
function parseIssueForm(body)
{
	const fields = new Map();
	if (!body)
		return fields;

	let current = null;
	let lines = [];
	let fence = null;
	const flush = () =>
	{
		if (current === null)
			return;
		const value = lines.join('\n').trim();
		fields.set(current, value === NO_RESPONSE ? '' : value);
	};

	for (const line of body.replace(/\r\n?/g, '\n').split('\n'))
	{
		if (fence === null)
		{
			const heading = /^### +(.+?)\s*$/.exec(line);
			if (heading)
			{
				flush();
				current = heading[1];
				lines = [];
				continue;
			}
		}

		const fenceMatch = /^ {0,3}(`{3,}|~{3,})/.exec(line);
		if (fenceMatch)
		{
			const marker = fenceMatch[1];
			if (fence === null)
				fence = marker;
			else if (marker[0] === fence[0] && marker.length >= fence.length && line.trim() === marker)
				fence = null;
		}

		if (current !== null)
			lines.push(line);
	}
	flush();
	return fields;
}

/** The answer without code fences, so an empty `render:` field (just its fence) counts as empty. */
function stripFences(value)
{
	return value.split('\n').filter((line) => !/^ {0,3}(`{3,}|~{3,})\S*\s*$/.test(line)).join('\n').trim();
}

/** Counts images and videos GitHub embedded in an answer (drag-and-drop uploads, markdown images, image links). */
function countAttachments(value)
{
	const pattern = /<img\b[^>]*>|<video\b[^>]*>|!\[[^\]]*\]\([^)]*\)|https?:\/\/github\.com\/user-attachments\/assets\/[^\s)"'<>]+|https?:\/\/\S+\.(?:png|jpe?g|gif|webp|mp4|mov)\b/gi;
	return (value.match(pattern) ?? []).length;
}

/** Whether an answer satisfies one of issue-triage.json's checks. */
function passesCheck(check, value)
{
	switch (check.rule)
	{
		case 'nonempty':
			return stripFences(value).length > 0;
		case 'pattern':
			return new RegExp(check.pattern).test(value);
		case 'attachment':
			return countAttachments(value) >= (check.min ?? 1);
		default:
			throw new Error(`Unknown check rule "${check.rule}" in issue-triage.json`);
	}
}

/** The options a dropdown answer selected: the whole answer if it's one option, else its comma-separated parts. */
function selectedOptions(value, options)
{
	if (Object.hasOwn(options, value))
		return [value];
	return value.split(/\s*,\s*/).filter((part) => Object.hasOwn(options, part));
}

/**
 * Works out what triage should do for an issue, without calling GitHub.
 * @param {{ body: string, labels: string[], authorAssociation: string, action: string }} issue
 * @param {{ fields: object, checks: object[] }} config issue-triage.json
 * @returns {{ addLabels: string[], missing: string[] }} labels to add (not already on the issue), and the `ask` of
 *   each check that failed
 */
function planTriage(issue, config)
{
	const fields = parseIssueForm(issue.body);
	const has = new Set(issue.labels.map((label) => label.toLowerCase()));
	const add = new Map();
	const addLabel = (label) =>
	{
		if (!has.has(label.toLowerCase()))
			add.set(label.toLowerCase(), label);
	};

	for (const [field, options] of Object.entries(config.fields))
	{
		const value = fields.get(field);
		if (!value)
			continue;
		for (const option of selectedOptions(value, options))
			options[option].forEach(addLabel);
	}

	if (issue.action === 'opened' && !TEAM_ASSOCIATIONS.has(issue.authorAssociation))
		addLabel(NEEDS_TRIAGE);

	const missing = [];
	for (const check of config.checks)
	{
		if (!check.labels.some((label) => has.has(label.toLowerCase())) || !fields.has(check.field))
			continue;
		if (!passesCheck(check, fields.get(check.field)) && !missing.includes(check.ask))
			missing.push(check.ask);
	}
	if (missing.length > 0)
		addLabel(NEEDS_INFO);

	return { addLabels: [...add.values()], missing };
}

function missingInfoComment(login, missing)
{
	return [
		MARKER,
		`Thanks for the report, @${login}. To reproduce and fix it, we still need:`,
		'',
		...missing.map((ask) => `- ${ask}`),
		'',
		'Please **edit the issue** (the `···` menu › *Edit*) to add them. This comment updates itself when you do.',
	].join('\n');
}

function completeComment()
{
	return [MARKER, COMPLETE_MARKER, 'Thanks, the report now has everything we asked for. A maintainer will take it from here.'].join('\n');
}

async function findTriageComment(github, owner, repo, issueNumber)
{
	const comments = await github.paginate(github.rest.issues.listComments, { owner, repo, issue_number: issueNumber, per_page: 100 });
	return comments.find((comment) => comment.user?.type === 'Bot' && comment.body?.includes(MARKER));
}

async function removeLabel(github, owner, repo, issueNumber, name)
{
	try
	{
		await github.rest.issues.removeLabel({ owner, repo, issue_number: issueNumber, name });
	}
	catch (error)
	{
		if (error.status !== 404)
			throw error;
	}
}

async function triageIssue({ github, context, core }, config)
{
	const { owner, repo } = context.repo;
	const issue = context.payload.issue;
	if (issue.state !== 'open')
	{
		core.info(`#${issue.number} is ${issue.state}; nothing to do.`);
		return;
	}

	const labels = issue.labels.map((label) => (typeof label === 'string' ? label : label.name));
	const plan = planTriage({ body: issue.body ?? '', labels, authorAssociation: issue.author_association, action: context.payload.action }, config);
	core.info(`#${issue.number}: adding [${plan.addLabels.join(', ')}]; missing ${plan.missing.length} detail(s)`);

	if (plan.addLabels.length > 0)
	{
		const spelled = await ensureLabels({ github, core }, owner, repo, plan.addLabels);
		await github.rest.issues.addLabels({ owner, repo, issue_number: issue.number, labels: spelled });
	}

	// Issue forms assign through their assignees: key; this covers blank issues and issues made through the API.
	const assignees = config.newIssues?.assignees ?? [];
	if (context.payload.action === 'opened' && (issue.assignees ?? []).length === 0 && assignees.length > 0)
	{
		core.info(`#${issue.number}: assigning ${assignees.join(', ')}`);
		await github.rest.issues.addAssignees({ owner, repo, issue_number: issue.number, assignees });
	}

	const existing = await findTriageComment(github, owner, repo, issue.number);
	if (plan.missing.length > 0)
	{
		const body = missingInfoComment(issue.user.login, plan.missing);
		if (!existing)
			await github.rest.issues.createComment({ owner, repo, issue_number: issue.number, body });
		else if (existing.body !== body)
			await github.rest.issues.updateComment({ owner, repo, comment_id: existing.id, body });
	}
	else if (existing && !existing.body.includes(COMPLETE_MARKER))
	{
		await github.rest.issues.updateComment({ owner, repo, comment_id: existing.id, body: completeComment() });
		if (labels.some((label) => label.toLowerCase() === NEEDS_INFO.toLowerCase()))
			await removeLabel(github, owner, repo, issue.number, NEEDS_INFO);
	}
}

async function triageComment({ github, context, core })
{
	const { owner, repo } = context.repo;
	const { issue, comment } = context.payload;
	const labels = issue.labels.map((label) => (typeof label === 'string' ? label : label.name));
	const waiting = labels.find((label) => label.toLowerCase() === NEEDS_INFO.toLowerCase());
	if (issue.state !== 'open' || !waiting || comment.user.type === 'Bot' || comment.user.login !== issue.user.login)
	{
		core.info('Not a reply from the reporter to an issue waiting for information; nothing to do.');
		return;
	}

	core.info(`#${issue.number}: the reporter replied; ${NEEDS_INFO} -> ${NEEDS_TRIAGE}`);
	await removeLabel(github, owner, repo, issue.number, waiting);
	const spelled = await ensureLabels({ github, core }, owner, repo, [NEEDS_TRIAGE]);
	await github.rest.issues.addLabels({ owner, repo, issue_number: issue.number, labels: spelled });
}

/** Entry point for actions/github-script. */
async function main({ github, context, core })
{
	const issue = context.payload.issue;
	if (!issue || issue.pull_request)
	{
		core.info('Not an issue event; nothing to do.');
		return;
	}

	if (context.eventName === 'issue_comment')
		await triageComment({ github, context, core });
	else
		await triageIssue({ github, context, core }, readGithubJson('issue-triage.json'));
}

module.exports = main;
module.exports.parseIssueForm = parseIssueForm;
module.exports.planTriage = planTriage;
module.exports.countAttachments = countAttachments;
