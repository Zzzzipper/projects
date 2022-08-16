import {
    authCompleted,
    authStatusLoaded,
    menuLoaded,
    moduleCreated,
    onLogout,
} from './actions';

export function auth(params: object) {
    return async (dispatch: any, _state: any, extra: any) => {
        const response = await extra.api.coreApi.auth(params);
        return dispatch(authCompleted(response));
    };
}

export function getAuthStatus(params: object) {
    return async (dispatch: any, _state: any, extra: any) => {
        const response = await extra.api.coreApi.getAuthStatus(params);
        return dispatch(authStatusLoaded(response));
    };
}

export function logout() {
    return async (dispatch: any, _state: any, extra: any) => {
        const response = await extra.api.coreApi.logout();
        return dispatch(onLogout(response));
    };
}

export function getMenu(param: any) {
    return async (dispatch: any, _state: any, extra: any) => {
        const response = await extra.api.coreApi.getMenu(param);
        return dispatch(menuLoaded(response));
    };
}

export function createModule() {
    return async (dispatch: any, _state: any, extra: any) => {
        const response = await extra.api.coreApi.createModule();
        return dispatch(moduleCreated(response));
    };
}
