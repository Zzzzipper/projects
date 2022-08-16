import { IRetrospectiveChart } from './retrospective-chart';

export interface IRetrospectiveConfiguration {
  caption: string,
  charts: IRetrospectiveChart[],
}
