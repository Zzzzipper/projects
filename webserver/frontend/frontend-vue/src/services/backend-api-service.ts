import urlcat from 'urlcat';
import {
  TBackendRequestAuthenticate,
  TBackendResponseAuthenticate,
  TBackendResponseAuthenticationStatus,
  TBackendResponseLogout,
  TBackendResponseMenu,
  TBackendResponseRetrospectiveMeasures,
  TBackendResponseRetrospectiveSettings,
  TBackendResponseLicenses
} from '../types';
import { IRetrospectiveValue } from '../entities/retrospective-value';
import { convertStringToBoolean, convertStringToNumber, uuid } from '../utils';
import { Color } from '../utilities/color';
import { IMenu } from '../entities/menu';
import { ILicense } from '../entities/licenses';
import { AuthenticationWrongCredentialsError } from '../errors/authentication-wrong-credentials-error';
import { AuthenticationUnknownError } from '../errors/authentication-unknown-error';
import { LogoutUnknownError } from '../errors/loguot-unknown-error';
import { IRetrospectiveMeasure } from '../entities/retrospective-measure';
import { IRetrospectiveChart } from '../entities/retrospective-chart';
import { convertBackendRetrospectiveMeasureStyleToEnum } from './backend-api-service-converters';

import { data as LicenseTestData } from '../testResponse/LicenseTest';

export default class BackendApiService {
  apiUrl: string;

  constructor(apiUrl: string) {
    this.apiUrl = apiUrl;
  }

  public async getRetrospectiveConfiguration(fileName: string):
    Promise<[IRetrospectiveChart[], IRetrospectiveMeasure[]]> {
    const url = this.buildUrl('/retro');
    const result = await this.request<TBackendResponseRetrospectiveSettings>(url, {
      method: 'post',
      credentials: 'include',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({
        file: 'Тестовая.rts_'
      })
    });

    const retrospectiveMeasures: IRetrospectiveMeasure[] = [];
    const retrospectiveCharts: IRetrospectiveChart[] = [];

    const charts = Array.isArray(result.retro.chart) ? result.retro.chart : [result.retro.chart];

    charts.forEach((chart) => {
      const measures = Array.isArray(chart.meas) ? chart.meas : [chart.meas];

      retrospectiveCharts.push({
        id: uuid(),
        name: chart['-name'],
        measureIds: measures.map(item => item['-id'])
      });

      measures.forEach((item) => {
        retrospectiveMeasures.push({
          id: item['-id'],
          name: `Measure ${item['-id']}`,
          color: (new Color(convertStringToNumber(item['-color']))).rgba,
          style: convertBackendRetrospectiveMeasureStyleToEnum(parseInt(item['-style'])),
          width: parseInt(item['-width']),
          isVisible: convertStringToBoolean(item['-visible']),
          values: []
        });
      });
    });

    return [retrospectiveCharts, retrospectiveMeasures];
  }

  public async getRetrospectiveMeasures(
    measuringId: string,
    startTime: number,
    endTime: number
  ): Promise<IRetrospectiveValue[]> {
    const url = this.buildUrl('/retro/measuring');
    const response = await this.request(url, {
      method: 'post',
      credentials: 'include',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({
        ids: [measuringId],
        startTime: Math.round(startTime / 1000),
        endTime: Math.round(endTime / 1000)
      })
    });

    const normalizedResponse = (response as string).replaceAll(/([0-9]+),([0-9]+)/gm, '$1.$2');
    const result = JSON.parse(normalizedResponse) as TBackendResponseRetrospectiveMeasures;
    const measuresEntites = result.map(item => ({
      date: new Date(item.DT * 1000),
      value: item.VAL
    }));
    measuresEntites.reverse();

    return measuresEntites;
  }

  public async getMenu(): Promise<IMenu> {
    const url = this.buildUrl('/menu');
    const response = await this.request<TBackendResponseMenu>(url, {
      method: 'post',
      credentials: 'include'
    });

    const menu: IMenu = {};

    response.menu.folder.forEach((folder) => {
      
      const isRetrospective = folder.file.some(file => file.command.includes('.rts_'));

      if (isRetrospective) {
        
        menu.retrospective = {
          title: folder.caption,
          items: folder.file.map(file => ({
            name: file.caption,
            fileId: file.command
          }))
        };
      }
    });

    return menu;
  }

  public async getMenuTest(): Promise<any> {
    const url = this.buildUrl('/request');
    const response = await this.request<TBackendResponseMenu>(url, {
      method: 'post',
      credentials: 'include',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({
        command: 'GetMenu',
        message: '',
      })
    });

    const menu: IMenu = {};

    return response;

    // response.menu.folder.forEach((folder) => {
    //   const isRetrospective = folder.file.some(file => file.command.includes('.rts_'));

    //   if (isRetrospective) {
    //     menu.retrospective = {
    //       title: folder.caption,
    //       items: folder.file.map(file => ({
    //         name: file.caption,
    //         fileId: file.command
    //       }))
    //     };
    //   }
    // });

    // return menu;
  }

  public async loadModule(): Promise<any> {
    const url = this.buildUrl('/request');
    const response = await this.request<TBackendResponseMenu>(url, {
      method: 'post',
      credentials: 'include',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({
        command: 'create_module',
        message: '{"library":"modSample","module":"sample1"}'
        // message: '{"library":"modRetroFull","module":"sample1"}'
      })
    });

    return response;
    
  }

  public async authenticate(login: string, password: string): Promise<void> {
    const url = this.buildUrl('/auth');
    const payload: TBackendRequestAuthenticate = { login, password };
    const response = await this.request<TBackendResponseAuthenticate>(url, {
      method: 'post',
      credentials: 'include',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify(payload)
    });

    // if ('status' in response) {
    //   response.code;
    // } else if ('error' in response) {
    //   throw new AuthenticationWrongCredentialsError();
    // }

    // throw new AuthenticationUnknownError();
    return;
  }

  public async getAuthenticationStatus(): Promise<boolean> {
    const url = this.buildUrl('/status');
    const response = await this.request<TBackendResponseAuthenticationStatus>(url, {
      method: 'post',
      credentials: 'include'
    });

    return response.loggedin === 'web';
  }

  public async logout(): Promise<void> {
    const url = this.buildUrl('logout');
    const response = await this.request<TBackendResponseLogout>(url, {
      method: 'post',
      credentials: 'include'
    });

    if ('error' in response || !('status' in response)) {
      throw new LogoutUnknownError();
    }
  }

  private buildUrl(path: string) {
    return urlcat(this.apiUrl, path);
  }

  private async request<T>(url: string, options?: RequestInit): Promise<T> {
    const response = await fetch(url, options);

    if (!response.ok) {
      throw new TypeError(`${response.status}: ${response.statusText}`);
    }

    return await response.json() as unknown as T;
  }

  public async getLicenseData(): Promise<ILicense> {
    // const url = this.buildUrl('/menu');
    // const response = await this.request<TBackendResponseLicenses>(url, {
    //   method: 'post',
    //   credentials: 'include'
    // });

    // return response;
    return LicenseTestData;
  }
}
