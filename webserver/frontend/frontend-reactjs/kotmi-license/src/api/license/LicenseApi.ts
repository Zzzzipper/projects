import BaseApi from '../BaseApi';
import queryTypes from '../data/queryTypes';

export default class CoreApi extends BaseApi {

    baseUrl = 'http://localhost:9000';

    async getLicense() {
        const url = `${this.baseUrl}/request`;
        const params = {
            command: 'License',
            message: '{"kotmi_license":{}}',
        };
        return this.sendQuery(url, params, queryTypes.post);
    }
}
