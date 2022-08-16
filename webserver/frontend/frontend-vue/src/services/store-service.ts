import { reactive } from 'vue';
import { IRetrospectiveMeasure } from '../entities/retrospective-measure';
import { IRetrospectiveChart } from '../entities/retrospective-chart';
import { IRetrospectiveValue } from '../entities/retrospective-value';
import { assert } from '../utils';

type TState = {
  retrospective: {
    measures: IRetrospectiveMeasure[],
    charts: IRetrospectiveChart[]
  }
};

export class StoreService {
  private static instance: StoreService | undefined;

  private internalState: TState = reactive({
    retrospective: {
      measures: [],
      charts: []
    }
  });

  public static getInstance () {
    if (this.instance === undefined) {
      this.instance = new StoreService();
    }

    return this.instance;
  }

  get state () {
    return this.internalState;
  }

  public setRetrospectiveMeasures (measures: IRetrospectiveMeasure[]) {
    this.internalState.retrospective.measures = measures;
  }

  public setRetrospectiveMeasureValues (measureId: string, values: IRetrospectiveValue[]) {
    const measure = this.internalState.retrospective.measures.find(item => item.id === measureId);
    assert(measure !== undefined, 'measure is not undefined');

    measure.values = values;
  }

  public updateRetrospectiveMeasure (measureId: string, fields: Omit<Partial<IRetrospectiveMeasure>, 'value'>) {
    const measure = this.internalState.retrospective.measures.find(item => item.id === measureId);
    assert(measure !== undefined, 'measure is not undefined');

    Object.assign(measure, fields);
  }

  public pushRetrospectiveMeasureValues (measureId: string, values: IRetrospectiveValue[]) {
    const measure = this.internalState.retrospective.measures.find(item => item.id === measureId);
    assert(measure !== undefined, 'measure is not undefined');

    measure.values.push(...values);
  }

  public spliceFromStartRetrospectiveMeasureValues (measureId: string, amount: number) {
    const measure = this.internalState.retrospective.measures.find(item => item.id === measureId);
    assert(measure !== undefined, 'measure is not undefined');

    measure.values.splice(0, amount);
  }

  public setRetrospectiveCharts (charts: IRetrospectiveChart[]) {
    this.internalState.retrospective.charts = charts;
  }
}

export function useStore () {
  return StoreService.getInstance();
}
