export interface IItem {
    name: string;
    caption: string;
    isValid: number;
    lastError: string;
};

export interface ILicenseParam extends IItem {
    value: number;
    currentValue: number;
};

export interface ILicenseItem extends IItem {
    param?: ILicenseParam[];
    license?: IItem[];
};

export interface ILicense {
    kotmi_license: {
        number: number;
        date: string;
        limit: number;
        version: string;
        vendor: {
            name: string;
        },
        client: {
            name: string;
        },
        licenses: ILicenseItem[];
    }
};
