import LicenseApi from './license';

interface IApplicationApi {
    licenseApi: LicenseApi;
};

class Api implements IApplicationApi {

    licenseApi: LicenseApi;

    constructor(baseUrl = '') {
        this.licenseApi = new LicenseApi(baseUrl);
    }
}

export default Api;
