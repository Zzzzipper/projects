import { CoreState } from './types';

const INITIAL_STATE: CoreState = {
    isLogout: {},
    loggedin: 'nothing',
    menu: {
        menu: [],
        pict: {},
    },
    module: {
        mod_id: null,
        obj_with_mod_id: null,
    }
};

export default INITIAL_STATE;
