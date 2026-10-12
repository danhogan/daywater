import { renderLayout } from './layout.js';

renderLayout();

// Project-specific facts stay in the isolated roadmap data, outside platform logic.
const statuses = { complete: 'Complete', partial: 'In progress', planned: 'Ahead' };
const text = (tag, value, className) => {
  const node = document.createElement(tag);
  node.textContent = value;
  if (className) node.className = className;
  return node;
};

try {
  const response = await fetch('/roadmaps/nfl-street.json', { cache: 'no-store' });
  if (!response.ok) throw new Error(`Roadmap HTTP ${response.status}`);
  const data = await response.json();
  for (const [id, value] of Object.entries({
    'roadmap-title': data.title,
    'roadmap-goal': data.goal,
    'roadmap-updated': `Evidence reviewed ${data.updated} · ${data.scope}`,
    'roadmap-current': data.current,
    'roadmap-next': data.next,
    'roadmap-why': data.why,
    'roadmap-boundary': data.boundary,
  })) document.getElementById(id).textContent = value;

  for (const capability of data.capabilities) {
    const card = document.createElement('div');
    card.append(text('h4', capability.title), text('p', capability.description));
    document.getElementById('roadmap-capabilities').append(card);
  }
  for (const [index, milestone] of data.milestones.entries()) {
    const item = document.createElement('li');
    const details = document.createElement('details');
    details.open = milestone.focus === true;
    const summary = document.createElement('summary');
    summary.append(text('span', `${index + 1}. ${milestone.title}`),
      text('span', statuses[milestone.status] || 'Unknown', `roadmap-status ${milestone.status}`));
    details.append(summary, text('p', milestone.description));
    for (const [label, value] of [
      ['Evidence', milestone.evidence], ['Complete when', milestone.acceptance],
      ['Still missing', milestone.remaining],
    ]) {
      const paragraph = document.createElement('p');
      paragraph.append(text('strong', `${label}: `), document.createTextNode(value));
      details.append(paragraph);
    }
    details.append(text('p', `Research notes: ${milestone.sources.join('; ')}`, 'roadmap-source'));
    item.append(details);
    document.getElementById('roadmap-milestones').append(item);
  }
  document.getElementById('roadmap').hidden = false;
} catch (error) {
  console.error('roadmap_load_failed', error);
  document.getElementById('roadmap-error').hidden = false;
}
