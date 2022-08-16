import React, { useState, useEffect } from 'react';

import { ToolbarButton, ToolbarBeginGroup, ToolbarDatePicker } from '../../components';
import { DATETIMEFORMAT_DATETIME, DATETIMEFORMAT_DATETIME_LONG } from '../../common/DateTimeConst';
import moment from 'moment';

import { AsureoTable } from './components';

import { createModule, getRequest, pingModule } from '../../common/Api';


export const AsureoViewer = () => {

    const [moduleDateTime, setModuleDateTime] = useState(moment().format(DATETIMEFORMAT_DATETIME_LONG));
    const [moduleData, setModuleData] = useState({
        id: 0,
        fields: [],
        data: [],
        filteredData: []
    });

    const [checkedButtons, setCheckedButtons] = useState({
        checkedAutoUpdate: true,
        checkedSelectResolutionZone: true
    });

    useEffect(() => {
        let moduleId = 0;
        let pingTimer = 0;

        // создание модуля
        createModule('modAsureo', 'modAsureo')
            .then(result => {
                console.log('code = ' + result);
                if (result.code >= 0) {
                    moduleId = result.body.mod_id;

                    pingTimer = setInterval(() => {
                        pingModule(moduleId);
                    }, 30000)

                    // запрос данных
                    getRequest(moduleId, 'ZVKBody', { time: Date.now() / 1000 }).then(result => {
                        //console.log('request result' + result)
                        if (result.code >= 0) {
                            if (result.body.result[0] === 0) {
                                // формирование списка имен полей
                                const newFields = result.body.fields.map(item => [getFieldName(item[0]), item[1]]);

                                // формирование данных 
                                const newData = getData(result.body.records, newFields);
                                //const newData = newData1.slice(0, 1);

                                // фильтрованные данные для грида
                                const newFilteredData = filterData(checkedButtons.checkedSelectResolutionZone, newData);

                                setModuleData({ ...moduleData, id: moduleId, fields: newFields, data: newData, filteredData: newFilteredData });
                            }
                            else {
                                console.log('ОШИБКА!!! ' + result.body.result)
                            }
                        }
                    })
                }
            });

        // действия при закрытии таба
        return () => {

            // удаление модуля данных
            if (moduleId > 0) {
                //console.log('destroy module');
            }

            if (pingTimer > 0) {
                console.log('clearInterval pingTimer');
                clearInterval(pingTimer);
            }
        }
    }, []);

    useEffect(() => {
        let moduleDateTimeTimer = 0;

        console.log('Изменение checkedAutoUpdate ' + checkedButtons.checkedAutoUpdate)
        if (checkedButtons.checkedAutoUpdate) {
            setModuleDateTime(getNow());
            moduleDateTimeTimer = setInterval(() => {
                setModuleDateTime(getNow());
            }, 1000);

        };

        return () => {
            if (moduleDateTimeTimer > 0)
                clearInterval(moduleDateTimeTimer);
        }
    }, [checkedButtons.checkedAutoUpdate]);


    useEffect(() => {
        let eventTimer = 0;
        if (moduleData.id > 0 && checkedButtons.checkedAutoUpdate) {
            eventTimer = setInterval(() => {
                loadChanges();
            }, 30000); // 5 мин = 300000
            console.log('eventTimer = ' + eventTimer)
        }
        return () => {
            if (eventTimer > 0) {
                console.log('eventTimer очистка ' + eventTimer)
                clearInterval(eventTimer);
            }
        }
    }, [checkedButtons.checkedAutoUpdate, moduleData.id]);


    useEffect(() => {
        console.log('data.Count = ' + moduleData.data.length);
    }, [moduleData.data]);

    useEffect(() => {
        console.log('filteredData.Count = ' + moduleData.filteredData.length);
    }, [moduleData.filteredData]);


    useEffect(() => {
        //console.log('время: ' + moduleDateTime);
    }, [moduleDateTime]);

    const getData = (arr, f = moduleData.fields) => {

        return arr.map(item => {
            const obj = {};
            item.forEach((element, index) => {
                const fieldName = f[index][0];
                obj[fieldName] = element;
            });
            return obj;
        });
    }

    const filterData = (checkedZone, filteringData) => {
        return filteringData.filter(item => (checkedZone && item.inResolutionZone) || (!checkedZone && item.inControlZone));
        /*
        return filteringData.filter(item => {
            const f = (checkedZone && item.inResolutionZone) || (!checkedZone && item.inControlZone);
            console.log(`${item.keyLink} ${item.inResolutionZone} ${item.inControlZone} f = ${f}`);
            return f;
        });*/
    }

    const getFieldName = (fieldName) => {
        return (fieldName === 'ZVKCategory') ? 'category' : fieldName[0].toLowerCase() + fieldName.slice(1);
    }

    const getNow = () => {
        return moment().format(DATETIMEFORMAT_DATETIME_LONG);
    }

    // изменение даты на календаре
    const onChange = (date, dateString) => {
        const d = moment(dateString, DATETIMEFORMAT_DATETIME_LONG);
        setModuleDateTime(dateString);
        loadData(+d);
    }

    // открытие календаря для смены даты
    const onOpenChange = (open) => {
        if (open && checkedButtons.checkedAutoUpdate)
            setCheckedButtons({ ...checkedButtons, checkedAutoUpdate: false });
    }


    const loadData = (d) => {
        console.log('loadData: ' + d / 1000);

        getRequest(moduleData.id, 'ZVKBody', { time: d / 1000 }).then(result => {
            if (result.code >= 0) {
                if (result.body.result[0] === 0) {
                    // формирование данных
                    const newData = getData(result.body.records);

                    // фильтрация
                    const newFilteredData = filterData(checkedButtons.checkedSelectResolutionZone, newData);

                    setModuleData({ ...moduleData, data: newData, filteredData: newFilteredData });
                }
            }
        });
    };

    const loadChanges = () => {
        console.log('load changes ' + moduleData.id);

        getRequest(moduleData.id, 'EventZVKBody', null).then(result => {
            if (result.code >= 0) {
                let insertedRecords = [];
                let editedRecords = [];
                let deletedRecords = [];

                // новые записи
                if (result.body.recordsinsert)
                    insertedRecords = getData(result.body.recordsinsert);

                // измененные записи
                if (result.body.recordsedit)
                    editedRecords = getData(result.body.recordsedit);

                // удаленные записи
                if (result.body.RecordsDelete)
                    result.body.RecordsDelete.forEach(item => deletedRecords.push(item[0]));

                // на случай, если не было изменений
                if (insertedRecords.length + editedRecords.length + deletedRecords.length > 0) {
                    console.log(`insertedRecords = ${insertedRecords.length} editedRecords = ${editedRecords.length} deletedRecords = ${deletedRecords.length}`)

                    setModuleData(oldModuleData => {
                        const showLog = 0;

                        // итоговая обработка данных
                        // 1. добавляем новые записи, если есть
                        let newData = [];
                        if (showLog > 0)
                            console.log('0. newData.count = ' + newData.length);

                        if (insertedRecords.length > 0)
                            newData = [...oldModuleData.data, ...insertedRecords]
                        else
                            newData = [...oldModuleData.data];
                        if (showLog > 0) {
                            console.log(`data.count = ${oldModuleData.data.length} insertedRecord.count = ${insertedRecords.length}`);
                            console.log('1. newData.count = ' + newData.length);
                        }

                        // 2. редактируем существующие записи, если были изменения
                        editedRecords.forEach(item => {
                            const dataItem = newData.find(e => e.keyLink === item.keyLink);
                            if (dataItem) {
                                for (let f = 1; f < oldModuleData.fields.length; f++) {
                                    const fieldName = oldModuleData.fields[f][0];
                                    dataItem[fieldName] = item[fieldName];
                                }
                            }
                        });
                        if (showLog > 0)
                            console.log('2. newData.count = ' + newData.length);

                        // 3. удаляем записи, если есть необходимость
                        if (deletedRecords.length > 0)
                            newData = newData.filter(item => !deletedRecords.includes(item.keyLink));
                        if (showLog > 0)
                            console.log('3. newData.count = ' + newData.length);

                        const newFilteredData = filterData(checkedButtons.checkedSelectResolutionZone, newData);
                        return { ...oldModuleData, data: newData, filteredData: newFilteredData };
                    });

                }
            }
        })
    };

    const onUpdateNowClick = (event) => {
        setModuleDateTime(getNow());
        setCheckedButtons({ ...checkedButtons, checkedAutoUpdate: false });
        loadData(Date.now());
    };

    const onAutoUpdateClick = (event) => {
        setCheckedButtons({ ...checkedButtons, checkedAutoUpdate: !checkedButtons.checkedAutoUpdate });
    };

    const onSelectResolutionZoneClick = (event) => {
        if (checkedButtons.checkedSelectResolutionZone)
            return;

        const checkedZone = true;
        setCheckedButtons({ ...checkedButtons, checkedSelectResolutionZone: checkedZone });
        const newFilteredData = filterData(checkedZone, moduleData.data);
        setModuleData({ ...moduleData, filteredData: newFilteredData });
    };

    const onSelectControlZoneClick = (event) => {
        if (!checkedButtons.checkedSelectResolutionZone)
            return;

        const checkedZone = false;
        setCheckedButtons({ ...checkedButtons, checkedSelectResolutionZone: checkedZone });
        const newFilteredData = filterData(checkedZone, moduleData.data);
        setModuleData({ ...moduleData, filteredData: newFilteredData });
    };

    const onReportClick = (event) => {
        console.log('ReportClick');
    };

    const onShowErrorsClick = (event) => {
        console.log('ShowErrorsClick');
    };

    return (
        <div className='tabcontent'>
            <div className='toolbar'>
                <ToolbarDatePicker caption='Время:' hint='Время просмотра' format={DATETIMEFORMAT_DATETIME_LONG}
                    value={moduleDateTime}
                    onChange={onChange} onOpenChange={onOpenChange}>
                </ToolbarDatePicker>
                <ToolbarButton icon='034 reload.png' hint='Обновить за текущее время'
                    onClick={onUpdateNowClick}
                ></ToolbarButton>
                <ToolbarButton icon='033 currenttime.png' hint='Автообновление'
                    checked={checkedButtons.checkedAutoUpdate} onClick={onAutoUpdateClick}></ToolbarButton>
                <ToolbarBeginGroup />
                <ToolbarButton icon='151 zone.png' caption='Зона ведения' hint='Зона ведения'
                    checked={checkedButtons.checkedSelectResolutionZone} onClick={onSelectResolutionZoneClick}></ToolbarButton>
                <ToolbarButton icon='151 zone.png' caption='Зона управления' hint='Зона управления'
                    checked={!checkedButtons.checkedSelectResolutionZone} onClick={onSelectControlZoneClick}>
                </ToolbarButton>
                <ToolbarBeginGroup />
                <ToolbarButton icon='212 report.png' hint='Построение отчета' onClick={onReportClick}></ToolbarButton>
                <ToolbarBeginGroup />
                <span>Всего заявок в зоне ответственности: {moduleData.filteredData.length}</span>
                <ToolbarButton icon='052 error.png' hint='Список ошибок' onClick={onShowErrorsClick}></ToolbarButton>
            </div>
            <AsureoTable data={moduleData.filteredData}>
            </AsureoTable>
        </div>
    )
}


/*
<Table
                bordered pagination={false} scroll={{ x: true }}
                size='small'
                columns={columns} dataSource={data} rowKey='keyLink'>
            </Table>
*/