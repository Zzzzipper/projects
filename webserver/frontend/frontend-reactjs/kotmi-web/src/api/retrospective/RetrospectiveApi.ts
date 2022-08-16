import BaseApi from '../BaseApi';
import queryTypes from '../data/queryTypes';
import { ObjKeys } from '../HttpActions';

export default class RetrospectiveApi extends BaseApi {

    baseUrl = 'http://localhost:8080';

    async createModule() {
        const url = `${this.baseUrl}/request`;
        const params = {
            command: 'create_module',
            message: '{"library":"modRetro","module":"retroFull"}',
        };
        return this.sendQuery(url, params, queryTypes.post);
    }

    async getRetro(param: ObjKeys) {
        const url = `${this.baseUrl}/request`;
        const params = {
            command: 'GetFormRetro',
            message: `{"name":"${param?.retroName}"}`,
            mod_id: param?.mod_id,
        };
        return this.sendQuery(url, params, queryTypes.post);
    }

    async getMeasures(param: ObjKeys) {
        const url = `${this.baseUrl}/request`;
        const params = {
            command: 'GetValues',
            message: `[{"id":"${param?.id}"}]`,
            mod_id: param?.mod_id,
        };
        return this.sendQuery(url, params, queryTypes.post);
    }
}
