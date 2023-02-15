import { data } from '../../../test_data/chart_types_eu.mjs';

const testSteps = [
    chart => chart.animate(
        {
            data: Object.assign(data, {
                filter: record =>
                    record.Country == 'Austria' ||
                    record.Country == 'Belgium' ||
                    record.Country == 'Bulgaria' ||
                    record.Country == 'Cyprus' ||
                    record.Country == 'Germany' ||
                    record.Country == 'Denmark'
            }),
            config: chart.constructor.presets.line({
                x: 'Year',
                y: 'Value 6 (+/-)',
                dividedBy: 'Country',
                title: 'Line Chart'
            })
        }
    )
];

export default testSteps;