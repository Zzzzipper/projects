import axios, { AxiosError, AxiosResponse } from 'axios';

export interface IApplicationResponse {
    data: object;
    status: number | undefined;
}

type ApplicationPromiseResponse =
    Promise<AxiosResponse<IApplicationResponse> | IApplicationResponse>;

export type ObjKeys = {
    [key: string]: any;
};

/**
 * Класс обертка на axios для обработки реквест параметров.
 */
class HttpActions {
    /**
     * Формирует полный url
     * @private
     */
    static getFullUrl(url: string, params: ObjKeys): string {
        let fullUrl = url;
        if (params) {
            let paramsArray = Object
                .keys(params)
                .filter((param) => ['', undefined, null].indexOf(params[param]) === -1);

            paramsArray = paramsArray.map(
                (param) => `${param}=${encodeURIComponent(params[param])}`,
            );

            fullUrl += `?${paramsArray.join('&')}`;
        }
        return fullUrl;
    }

    static getErrorObject(error: AxiosError): IApplicationResponse {
        const { response } = error;
        return {
            status: response?.status,
            data: response?.data,
        };
    }

    /** Функция выполняет Get запрос на сервер */
    static get(url: string, data: ObjKeys): ApplicationPromiseResponse {
        const urlWithArguments = HttpActions.getFullUrl(url, data);
        return axios.get(urlWithArguments).catch((e) => HttpActions.getErrorObject(e));
    }

    /** Функция выполняет Post запрос на сервер */
    static post(url: string, data: object): ApplicationPromiseResponse {
        return axios.post(url, data, { withCredentials: true })
            .catch((e) => HttpActions.getErrorObject(e));
    }

    /** Функция выполняет Put запрос на сервер */
    static put(url: string, data: object): ApplicationPromiseResponse {
        return axios.put(url, data).catch((e) => HttpActions.getErrorObject(e));
    }

    /** Функция выполняет Remove запрос на сервер */
    static delete(url: string, data: object): ApplicationPromiseResponse {
        return axios.delete(url, data).catch((e) => HttpActions.getErrorObject(e));
    }
}

export default HttpActions;
