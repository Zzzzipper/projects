import * as types from './constants';
import { action } from 'typesafe-actions';

export const licenseLoaded = (response: any) => action(types.GET_LICENSE, response);
