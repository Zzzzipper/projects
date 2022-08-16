import * as types from './constants';
import { action } from 'typesafe-actions';

export const moduleCreated = (response: any) => action(types.CREATE_MODULE, response);
export const retroLoaded = (response: any) => action(types.GET_RETRO, response);
export const measuresLoaded = (response: any) => action(types.GET_MEASURES, response);
