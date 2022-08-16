export interface IList {
    name: string;
    caption: string;
    isValid: number;
    lastError: string;
};

export interface IParam extends IList {
    value: number;
    currentValue: number;
};

export interface ILicense extends IList {
    param?: Array<IParam>;
    license?: Array<IList>;
};

export interface IMainLicense {
    number: number;
    date: string;
    limit: number;
    version: string;
    vendor: {
        name: string;
    };
    client: {
        name: string;
    };
    licenses: Array<ILicense>;
}

export type ILicenseState = {
    kotmi_license: IMainLicense;
} | undefined;

export type LicenseState = Readonly<{
    license?: {
        kotmi_license: IMainLicense
    };
}>;
