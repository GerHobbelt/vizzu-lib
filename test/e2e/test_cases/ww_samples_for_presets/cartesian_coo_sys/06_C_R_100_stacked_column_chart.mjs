import { data } from '../../../test_data/chart_types_eu.mjs'

const testSteps = [
	(chart) =>
		chart.animate({
			data: Object.assign(data, {
				filter: (record) =>
					record.Country === 'Austria' ||
					record.Country === 'Belgium' ||
					record.Country === 'Bulgaria' ||
					record.Country === 'Cyprus' ||
					record.Country === 'Czechia' ||
					record.Country === 'Denmark'
			}),
			config: {
				channels: {
					x: 'Country',
					y: { set: ['Joy factors', 'Value 2 (+)'], align: 'stretch' },
					color: 'Joy factors',
					label: 'Value 2 (+)'
				},
				title: 'Percentage Stacked Column Chart'
			}
		}),
	(chart) => {
		chart.feature('tooltip', true)
		return chart
	}
]

export default testSteps
