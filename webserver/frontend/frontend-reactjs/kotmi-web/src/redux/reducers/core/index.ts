import * as types from '../../actions/core/constants';
import { Reducer } from 'redux';
import { CoreState } from './types';
import INITIAL_STATE from './initial';
import { CoreAction } from '../../../redux/actions/core/types';

const coreReducer: Reducer<CoreState, CoreAction> = (state = INITIAL_STATE, action) => {
    const { payload = {}, type } = action;
    const { data = {} } = payload;

    switch (type) {
        case types.AUTH:
        case types.AUTH_STATUS: {
            return {
                ...state,
                loggedin: (data?.code === 9) ? 'web' : 'nothing' || 'nothing',
            };
        }
        case types.ON_LOGOUT:
            return {
                ...state,
                isLogout: data,
            };
        case types.CREATE_MODULE: {
            return {
                ...state,
                module: data?.body || {
                    mod_id: null,
                    obj_with_mod_id: null,
                },
            }
        }
        case types.GET_MENU:
            return {
                ...state,
                menu: data?.body || {},
            }
        default:
            return state;
    }
};

export default coreReducer;
