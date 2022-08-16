import HttpActions, { ObjKeys } from './HttpActions';
import queryTypes, { ActionType } from './data/queryTypes';

export default class BaseApi {

    baseUrl = '';

    /**
     * @constructor
     * @param {String} baseUrl
     */
    constructor(baseUrl: string) {
        this.baseUrl = baseUrl;
    }

    responseHandler(response: any) {
        const { status } = response;
        if (!status) {
            return;
        }
        let level = '';

        switch (status) {
            case 200:
            case 201: {
                level = 'success';
                break;
            }
            case 401: {
                break;
            }
            case 403:
            case 404:
            case 409:
            case 410: {
                level = 'warning';
                break;
            }
            case 400:
            case 500: {
                level = 'error';
                break;
            }
            case 510: {
                level = 'error';
                break;
            }
            default:
        }
    }

    async sendQuery(url: string, data: ObjKeys, method: ActionType) {
        const { post, get, put } = HttpActions;

        const responseMethods = {
            [queryTypes.post]: async () => post(url, data),
            [queryTypes.get]: async () => get(url, data),
            [queryTypes.put]: async () => put(url, data),
            [queryTypes.delete]: async () => HttpActions.delete(url, data),
        };

        const response = await responseMethods[method]();

        this.responseHandler(response);

        const sucessCodes = Array<number | undefined>(200, 201);

        return {
            data: response.data,
            status: response.status,
            success: sucessCodes.includes(response.status),
        };
    }
}
