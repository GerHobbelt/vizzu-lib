import { data } from '../../../../../test_data/chart_types_eu.mjs';


const testSteps = [
    chart => chart.animate({
        data: Object.assign(data, {
            filter: record =>
                record.Country == 'Austria' ||
                record.Country == 'Belgium' ||
                record.Country == 'Bulgaria' ||
                record.Country == 'Cyprus' ||
                record.Country == 'Czechia' ||
                record.Country == 'Denmark' ||
                record.Country == 'Estonia' ||
                record.Country == 'Greece' ||
                record.Country == 'Germany' ||
                record.Country == 'Spain' ||
                record.Country == 'Finland' ||
                record.Country == 'France' ||
                record.Country == 'Croatia' ||
               record.Country == 'Hungary'
        }),
        config: {
            channels: {
                x: { set: 'Year' },
                y: { set: 'Value 2 (+)' },
                color: { set: 'Joy factors' }
            },
            title: 'Line Chart',
            geometry: 'line'
        } 
    }),
    chart => chart.animate({
        config: {
            channels: {
                x: { set: 'Year' },
                y: { set: ['Joy factors'] },
                color: { set: 'Joy factors' }
            },
            title: 'Stack Disc & Remove Conti'
        }
    }
    ),

    chart => chart.animate({
        config: {
            channels: {
                x: { set: 'Country_code' },
                y: { set: 'Joy factors' },
                color: { set: 'Joy factors' }
            },
            title: 'Change Disc'
        }
    },
    {
             duration: 0.5
         }
    ),

chart => chart.animate({
    config: {
        channels: {
            x: { set: null },
            y: { set: null },
            color: { set: 'Joy factors' },
            size: { set: ['Country_code', 'Value 2 (+)'] }
        },
        title: 'Add previous Conti & Change Geoms & CoordSys',
        geometry: 'circle'
    }
},
{
  x: {
      delay: 0.5
  }
 }
)];

export default testSteps;