import { RetrospectiveChartBroker } from '../services/retrospective-chart-broker';

export interface IRetrospectiveChart {
  id: string,
  name: string,
  measureIds: string[],
  broker?: RetrospectiveChartBroker
}
