import { useStore } from '../services/store-service';
import { assert } from '../utils';

export class RetrospectiveCrosshairsSetCoordinatesInteractor {
  public static async execute (chartId: string, x: number, y: number) {
    const store = useStore();
    const chart = store.state.retrospective.charts.find(item => item.id === chartId);
    assert(chart !== undefined, 'chart is undefined');
    assert(chart.broker !== undefined, 'chart.broker is undefined');
    await chart.broker.setCrosshairCoordinates(x, y);
  }
}
