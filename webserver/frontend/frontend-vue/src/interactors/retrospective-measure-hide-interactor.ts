import { useStore } from '../services/store-service';
import { assert } from '../utils';

export class RetrospectiveMeasureHideInteractor {
  public static async execute (measureId: string) {
    const store = useStore();

    store.updateRetrospectiveMeasure(measureId, {
      isVisible: false
    });

    const chart = store.state.retrospective.charts.find(chart => chart.measureIds.includes(measureId));
    assert(chart !== undefined, 'chart is undefined');

    await chart.broker?.hideLine(measureId);
    await chart.broker?.render();
  }
}
