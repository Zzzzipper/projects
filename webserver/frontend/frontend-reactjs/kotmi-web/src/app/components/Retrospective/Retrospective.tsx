import React from 'react';
import { RouteComponentProps } from 'react-router';

import moment from 'moment';
import { Color } from '../../utils/color';
import { convertStringToNumber } from '../../utils/converter';

import { AppState } from '../../../redux/reducers';
import { connect } from 'react-redux';

import {
   createModule,
   getRetro,
   getMeasures,
} from '../../../redux/actions/retrospective';

import {
   TMeas,
   TModule,
   TRetrospecive,
} from '../../../redux/reducers/retrospective/types';

import { Line } from 'react-chartjs-2';
import {
   Chart as ChartJS,
   CategoryScale,
   LinearScale,
   PointElement,
   LineElement,
   Title,
   Tooltip,
   Legend,
} from 'chart.js';

import RetrospectivePanel from './RetrospectivePanel';

interface IMatchParams {
   id: string;
}

interface IProps {
   retrospective: TRetrospecive;
   module: TModule;
}

interface IDispatchProps {
   createModule: (param: object) => Promise<any>;
   getRetro: (param: object) => Promise<any>;
   getMeasures: (param: object) => Promise<any>;
}

class Retrospective extends React.Component<IProps & IDispatchProps & RouteComponentProps<IMatchParams>> {

   componentDidMount() {
      this.registerChart();

      this.props.createModule({}).then((response) => {
         const param = {
            mod_id: this.props.module?.mod_id,
            retroName: this.props.match.params.id,
         }
         this.props.getRetro(param).then(this.getMeasures);
      });
   }

   componentDidUpdate(prevProps: any): any {
      if (this.props.match.params.id !== prevProps.match.params.id) {
         const param = {
            mod_id: this.props.module?.mod_id,
            retroName: this.props.match.params.id,
         }
         this.props.getRetro(param);
      }
   }

   registerChart = () => {
      ChartJS.register(
         CategoryScale,
         LinearScale,
         PointElement,
         LineElement,
         Title,
         Tooltip,
         Legend,
      );
   };

   createLabels = () => {
      const labels = new Array();
      for (let i = 0; i != 25; ++i) {
         labels.push(moment().add(i, 'minute').format('HH:mm:ss.SSS'));
      }
      return labels;
   };

   createOptions = () => {
      const options = {
         responsive: true,
         plugins: {
            legend: {
               position: 'top' as const,
            },
            title: {
               display: false,
            },
         },
         scales: {
            xAxes: {
               ticks: {
                  autoSkip: false,
                  maxRotation: 90,
                  minRotation: 90,
               },
            },
         },
      };
      return options;
   };

   randomDataTest = () => {
      const data = new Array();
      const min = -10;
      const max = 10;
      for (let i = 0; i != 25; ++i) {
         data.push(Math.random() * (max - min) + min);
      }
      return data;
   }

   createData = (meas: TMeas[]) => {
      const labels = this.createLabels();
      const data = {
         labels: labels,
         datasets: meas.map((meas) => {
            const color = new Color(convertStringToNumber(meas.attrib.color)).rgba;
            return {
               label: meas.attrib.id,
               backgroundColor: color,
               borderColor: color,
               data: this.randomDataTest(),
            }
         })
      };
      return data;
   };

   getMeasures = () => {
      const measures = new Array();
      this.props.retrospective?.content.map((retro) => {
         retro.content.map((meas) => {
            measures.push({id: meas.attrib.id});
         });
      });
      console.log(measures);
   };

   render() {
      return (
         <div className='App-body'>
            <RetrospectivePanel />
            <h2>{this.props.retrospective?.attrib.caption}</h2>
            {this.props.retrospective?.content.map((retro, index) => {
               const options = this.createOptions();
               const data = this.createData(retro.content);
               return <div className='chart' key={index}><Line options={options} data={data} /></div>
            })}
         </div>
      )
   }
}

function mapStateToProps(state: AppState): IProps {
   return {
      retrospective: state.retro.retrospective,
      module: state.retro.module,
   };
}

const mapDispatchToProps = {
   createModule,
   getRetro,
   getMeasures,
};

export default connect(
   mapStateToProps,
   mapDispatchToProps,
)(Retrospective);
