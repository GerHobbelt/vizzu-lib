import { loadAnimation } from '../../../../../docs/assets/javascripts/snippet.js'

async function getTestSteps(dataFile, configName) {
	const dataLoaded = import(dataFile)
	const configLoaded = import(`../../../../../docs/tutorial/${configName}/config.js`)
	const [data, config] = await Promise.all([dataLoaded, configLoaded])

	const baseUrl = {
		nodeBaseUrl: `${import.meta.url.replace('file://', '').replace('/events.mjs', '')}/../../../../../docs/tutorial/${configName}`,
		browserBaseUrl: `../../../../../docs/tutorial/${configName}`
	}

	const steps = []
	for (const animation of config.default) {
		for (const subAnimation of animation.anims) {
			const func = await loadAnimation(
				`${subAnimation.name}.js`,
				Object.assign({}, subAnimation, baseUrl)
			)
			steps.push((chart) => func(chart, data.default, animation?.assets))
		}
	}

	steps.unshift((chart) => chart.animate({ data: data.default }))
	return steps
}

const testSteps = await getTestSteps('../../../../../docs/assets/data/music_data.js', 'events')
export default testSteps
