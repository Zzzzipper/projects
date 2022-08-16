import {
    retroLoaded,
    moduleCreated,
    measuresLoaded,
} from './actions';

export function createModule() {
    return async (dispatch: any, _state: any, extra: any) => {
        const response = await extra.api.retrosectiveApi.createModule();
        return dispatch(moduleCreated(response));
    };
}

export function getRetro(param: object) {
    return async (dispatch: any, _state: any, extra: any) => {
        const response = await extra.api.retrosectiveApi.getRetro(param);
        return dispatch(retroLoaded(response));
    };
}

export function getMeasures(param: object) {
    return async (dispatch: any, _state: any, extra: any) => {
        const response = await extra.api.retrosectiveApi.getMeasures(param);
        return dispatch(measuresLoaded(response));
    };
}
