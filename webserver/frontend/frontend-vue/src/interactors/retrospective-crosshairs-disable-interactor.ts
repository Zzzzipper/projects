import { useStore } from '../services/store-service';
import { assert } from '../utils';

export class RetrospectiveCrosshairsDisableInteractor {
  public static async execute (chartId: string) {
    const store = useStore();
    const chart = store.state.retrospective.charts.find(item => item.id === chartId);
    assert(chart !== undefined, 'chart is undefined');
    assert(chart.broker !== undefined, 'chart.broker is undefined');
    await chart.broker.disableCrosshair();
  }
}
