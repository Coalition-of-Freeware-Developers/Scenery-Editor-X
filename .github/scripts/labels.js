// Label helpers shared by issue_triage.js (issue-triage.yml) and sync-labels.yml, run through actions/github-script.
// The label list lives in .github/labels.json; validate_github_forms.py checks it.
'use strict';

const fs = require('fs');
const path = require('path');

const GITHUB_DIR = path.join(__dirname, '..');

/** Reads a JSON file from the .github folder. */
function readGithubJson(file)
{
	return JSON.parse(fs.readFileSync(path.join(GITHUB_DIR, file), 'utf8'));
}

/** The labels in .github/labels.json, keyed by lower-case name. */
function readLabelDefinitions()
{
	const definitions = new Map();
	for (const label of readGithubJson('labels.json').labels)
		definitions.set(label.name.toLowerCase(), label);
	return definitions;
}

/** Every label in the repository, keyed by lower-case name. */
async function listRepositoryLabels(github, owner, repo)
{
	const labels = await github.paginate(github.rest.issues.listLabelsForRepo, { owner, repo, per_page: 100 });
	return new Map(labels.map((label) => [label.name.toLowerCase(), label]));
}

/**
 * Creates the labels in `names` the repository doesn't have yet, with the colour and description from labels.json
 * (GitHub would otherwise create them grey and without a description when they're added to an issue).
 * Returns the names as the repository spells them.
 */
async function ensureLabels({ github, core }, owner, repo, names)
{
	if (names.length === 0)
		return [];

	const definitions = readLabelDefinitions();
	const existing = await listRepositoryLabels(github, owner, repo);
	const spelled = [];
	for (const name of names)
	{
		const key = name.toLowerCase();
		const present = existing.get(key);
		if (present)
		{
			spelled.push(present.name);
			continue;
		}

		const definition = definitions.get(key) ?? { name, color: 'ededed', description: '' };
		try
		{
			await github.rest.issues.createLabel({ owner, repo, name: definition.name, color: definition.color, description: definition.description ?? '' });
			core.info(`Created label "${definition.name}"`);
		}
		catch (error)
		{
			// 422: another run created it a moment ago.
			if (error.status !== 422)
				throw error;
		}
		spelled.push(definition.name);
	}
	return spelled;
}

/**
 * Makes the repository's labels match labels.json: creates missing labels and fixes colours and descriptions.
 * Never deletes or renames a label, so labels made by hand stay.
 */
async function syncLabels({ github, context, core })
{
	const { owner, repo } = context.repo;
	const existing = await listRepositoryLabels(github, owner, repo);
	const created = [];
	const updated = [];

	for (const definition of readLabelDefinitions().values())
	{
		const present = existing.get(definition.name.toLowerCase());
		const description = definition.description ?? '';
		if (!present)
		{
			await github.rest.issues.createLabel({ owner, repo, name: definition.name, color: definition.color, description });
			created.push(definition.name);
		}
		else if (present.color.toLowerCase() !== definition.color.toLowerCase() || (present.description ?? '') !== description)
		{
			await github.rest.issues.updateLabel({ owner, repo, name: present.name, color: definition.color, description });
			updated.push(present.name);
		}
	}

	core.info(`Created ${created.length} label(s): ${created.join(', ') || 'none'}`);
	core.info(`Updated ${updated.length} label(s): ${updated.join(', ') || 'none'}`);
	await core.summary
		.addHeading('Label sync')
		.addRaw(`Created: ${created.join(', ') || 'none'}`, true)
		.addRaw(`Updated: ${updated.join(', ') || 'none'}`, true)
		.write();
}

module.exports = { readGithubJson, readLabelDefinitions, listRepositoryLabels, ensureLabels, syncLabels };
