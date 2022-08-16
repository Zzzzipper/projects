import BaseApi from '../BaseApi';
import queryTypes from '../data/queryTypes';
import { ObjKeys } from '../HttpActions';

export default class CoreApi extends BaseApi {

    baseUrl = 'http://localhost:8080';

    async getAuthStatus() {
        console.log(this.baseUrl);
        const url = `${this.baseUrl}/status`;
        return this.sendQuery(url, {}, queryTypes.post);
    }

    async auth(params: ObjKeys) {
        const url = `${this.baseUrl}/login`;
        return this.sendQuery(url, params, queryTypes.post);
    }

    async logout() {
        const url = `${this.baseUrl}/logout`;
        return this.sendQuery(url, {}, queryTypes.post);
    }

    async createModule() {
        const url = `${this.baseUrl}/request`;
        const params = {
            command: 'create_module',
            message: '{"library":"modSample","module":"sample1"}',
            mod_id: 1,
        };
        return this.sendQuery(url, params, queryTypes.post);
    }

    async getMenu(mod_id: number) {
        const url = `${this.baseUrl}/request`;
        const params = {
            command: 'GetMenu',
            message: '',
            mod_id,
        };
        return this.sendQuery(url, params, queryTypes.post);
    }

    
}
