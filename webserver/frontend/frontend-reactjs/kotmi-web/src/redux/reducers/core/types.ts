export type TAuthStatus = 'web' | 'nothing' | undefined;

export type TLogout = {
    error?: string,
    status?: string,
} | undefined;

export type IFile = {
    caption?: string;
    command?: string;
    ico?: string;
    oneCopy?: number;
    type?: number;
}| undefined;

export type IFolder = {
    caption?: string;
    ico?: string;
    menu?: Array<IFile>;
} | undefined;

export type IMenu = {
    menu: Array<IFolder>;
    pict: object;
} | undefined;

export type TModule = {
    mod_id: number | null;
    obj_with_mod_id: number | null;
} | undefined;

export type CoreState = Readonly<{
    loggedin?: TAuthStatus;
    isLogout?: TLogout;
    menu?: IMenu | undefined;
    module?: TModule;
}>;
