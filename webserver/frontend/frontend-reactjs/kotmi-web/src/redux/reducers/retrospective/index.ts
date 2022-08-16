import * as types from '../../actions/retrospective/constants';
import { Reducer } from 'redux';
import { RetrospectiveState } from './types';
import INITIAL_STATE from './initial';
import { RetrospectiveAction } from '../../../redux/actions/retrospective/types';

const retroReducer: Reducer<RetrospectiveState, RetrospectiveAction> = (state = INITIAL_STATE, action) => {
    const { payload = {}, type } = action;
    const { data = {} } = payload;

    switch (type) {
        case types.CREATE_MODULE:
            return {
                ...state,
                module: data?.body || {
                    mod_id: null,
                    obj_with_mod_id: null,
                },
            };
        case types.GET_RETRO: {
            return {
                ...state,
                retrospective: data?.body || {},
            };
        }
        case types.GET_MEASURES: {
            return {
                ...state,
            };
        }
        default:
            return state;
    }
};

export default retroReducer;
