import CoreApi from './core';
import RetrosectiveApi from './retrospective';

interface IApplicationApi {
    coreApi: CoreApi;
    retrosectiveApi: RetrosectiveApi;
};

class Api implements IApplicationApi {

    coreApi: CoreApi;
    retrosectiveApi: RetrosectiveApi;

    constructor(baseUrl = '') {
        this.coreApi = new CoreApi(baseUrl);
        this.retrosectiveApi = new RetrosectiveApi(baseUrl);
    }
}

export default Api;
