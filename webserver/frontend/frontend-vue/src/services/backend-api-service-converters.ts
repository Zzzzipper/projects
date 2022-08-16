import { ERetrospectiveMeasureStyle } from '../types';

export function convertBackendRetrospectiveMeasureStyleToEnum (value: number) {
  switch (value) {
    case 0:
      return ERetrospectiveMeasureStyle.SOLID;
    case 1:
      return ERetrospectiveMeasureStyle.DASHED_LONG_LINES;
    case 2:
      return ERetrospectiveMeasureStyle.DASHED_SHORT_LINES;
    case 3:
      return ERetrospectiveMeasureStyle.DASHED_VARIABLE_LENGTH_LINES;
    default:
      throw new Error('unknown retrospective measure style');
  }
}
