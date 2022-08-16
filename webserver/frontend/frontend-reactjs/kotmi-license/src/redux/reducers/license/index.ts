import * as types from '../../actions/license/constants';
import { Reducer } from 'redux';
import { LicenseState } from './types';
import INITIAL_STATE from './initial';
import { LicenseAction } from '../../../redux/actions/license/types';

const licenseReducer: Reducer<LicenseState, LicenseAction> = (state = INITIAL_STATE, action) => {
    const { payload = {}, type } = action;
    const { data = {} } = payload;

    switch (type) {
        case types.GET_LICENSE: {
            return {
                ...state,
                license: data?.body || {},
            };
        }
        default:
            return state;
    }
};

export default licenseReducer;
