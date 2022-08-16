import { ERetrospectiveMeasureStyle } from '../types';
import { IRetrospectiveValue } from './retrospective-value';

export interface IRetrospectiveMeasure {
  id: string;
  name: string;
  color: string;
  style: ERetrospectiveMeasureStyle,
  width: number;
  isVisible: boolean;
  values: IRetrospectiveValue[]
}
