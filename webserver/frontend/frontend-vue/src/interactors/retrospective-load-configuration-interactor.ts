import BackendApiService from '../services/backend-api-service';
import config from '../config';
import { StoreService } from '../services/store-service';

export class RetrospectiveLoadConfigurationInteractor {
  public static async execute (retrospectiveId: string) {
    const backendApiService = new BackendApiService(config.backendApiUrl);
    const [retrospectiveCharts, retrospectiveMeasures] = (
      await backendApiService.getRetrospectiveConfiguration(retrospectiveId)
    );

    StoreService.getInstance().setRetrospectiveCharts(retrospectiveCharts);
    StoreService.getInstance().setRetrospectiveMeasures(retrospectiveMeasures);
  }
}
