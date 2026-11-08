// Adds an issue to the GitHub project named in .github/issue-triage.json (newIssues.project).
// Run by .github/workflows/add-to-project.yml through actions/github-script, with a token that can write the project.
//
// Not actions/add-to-project: when the repository's owner differs from the project's (this repository belongs to a
// user, the project to the Coalition-of-Freeware-Developers organization) it adds a draft issue holding only the
// issue's URL instead of the issue itself. addProjectV2ItemById links the real issue across owners.
'use strict';

const { readGithubJson } = require('./labels.js');

const PROJECT_URL = /^https:\/\/github\.com\/(orgs|users)\/([^/]+)\/projects\/(\d+)\/?$/;

/** The project's node ID and title from its URL. */
async function findProject(github, url)
{
	const match = PROJECT_URL.exec(url);
	if (!match)
		throw new Error(`newIssues.project must look like https://github.com/orgs/<org>/projects/<number>, not "${url}"`);

	const ownerField = match[1] === 'orgs' ? 'organization' : 'user';
	const response = await github.graphql(
		`query($login: String!, $number: Int!) { ${ownerField}(login: $login) { projectV2(number: $number) { id title } } }`,
		{ login: match[2], number: Number(match[3]) });
	const project = response[ownerField]?.projectV2;
	if (!project)
		throw new Error(`Can't see the project ${url}: check that the token has read and write access to it`);
	return project;
}

/** Entry point for actions/github-script. `issueNumber` comes from workflow_dispatch; issue events use their payload. */
async function main({ github, context, core }, issueNumber)
{
	const { owner, repo } = context.repo;
	let issue = context.payload.issue;
	if (!issue)
		issue = (await github.rest.issues.get({ owner, repo, issue_number: Number(issueNumber) })).data;
	if (issue.pull_request)
	{
		core.info(`#${issue.number} is a pull request; skipped.`);
		return;
	}

	const url = readGithubJson('issue-triage.json').newIssues?.project;
	if (!url)
	{
		core.info('No newIssues.project in issue-triage.json; nothing to do.');
		return;
	}

	const project = await findProject(github, url);
	try
	{
		const response = await github.graphql(
			`mutation($project: ID!, $content: ID!) { addProjectV2ItemById(input: { projectId: $project, contentId: $content }) { item { id } } }`,
			{ project: project.id, content: issue.node_id });
		core.info(`Added #${issue.number} to "${project.title}" (item ${response.addProjectV2ItemById.item.id}).`);
	}
	catch (error)
	{
		if (/already exists/i.test(error.message ?? ''))
		{
			core.info(`#${issue.number} is already in "${project.title}".`);
			return;
		}
		throw error;
	}
}

module.exports = main;
