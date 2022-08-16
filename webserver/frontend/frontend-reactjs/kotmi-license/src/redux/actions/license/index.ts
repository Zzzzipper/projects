import {
    licenseLoaded,
} from './actions';

export function getLicense(params: object) {
    return async (dispatch: any, _state: any, extra: any) => {
        const response = await extra.api.licenseApi.getLicense(params);
        return dispatch(licenseLoaded(response));
    };
}
