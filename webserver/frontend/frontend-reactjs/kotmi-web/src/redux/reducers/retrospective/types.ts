export type TModule = {
    mod_id: number | null;
    obj_with_mod_id: number | null;
} | undefined;

export type TRetroCommon = {
    align: string,
    alwaysrenewal: string,
    caption: string,
    datamode: string,
    elementnamewithlocid: string,
    formattime: string,
    interval: string,
    showtab: string,
    step: string,
    tableorientation: string,
    timeorientation: string,
};

export type TMeasAttrib = {
    align: string,
    color: string,
    formatvalue: string,
    id: string,
    linestep: string,
    pointer: string,
    presentationtype: string,
    shift: string,
    style: string,
    visible: string,
    width: string,
};

export type TMeas = {
    attrib: TMeasAttrib,
    name: string,
};

export type TRetroAttrib = {
    backgroundcolor: string,
    charttype: string,
    gradientdirection: string,
    gradientendcolor: string,
    gradientstartcolor: string,
    gradientvisible: string,
    legendalignment: string,
    legendcolor: string,
    legendvisible: string,
    limitautomatic: string,
    limitmaximum: string,
    limitminimum: string,
    name: string,
    view3d: string,
}

export type TRetro = {
    attrib: TRetroAttrib,
    content: TMeas[],
    name: string,
};

export type TRetrospecive = {
    attrib: TRetroCommon,
    content: TRetro[],
    name: string,
} | undefined;

export type RetrospectiveState = Readonly<{
    retrospective?: TRetrospecive;
    module?: TModule;
}>;