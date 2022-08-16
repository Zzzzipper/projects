import React, { useEffect, useState } from 'react';
import { Table } from 'antd';

const sortDirections = ['ascend', 'descend', 'ascend'];
const defaultSortOrder = 'ascend';

export const AsureoTable = ({ data }) => {

    const [options, setOptions] = useState({
        sortedInfo: { columnKey: 'selfNumber', order: 'ascend' }, // сортировка первого столбца при открытии модуля
        filteredInfo: {}
    });

    useEffect(() => {
        console.log('sortedInfo changed')
    }, [options.sortedInfo]);

    useEffect(() => {
        console.log('filteredInfo changed')
    }, [options.filteredInfo]);

    useEffect(() => {
        console.log('data changed')
    }, [data]);


    const columns = [
        /*
        {
            title: 'KeyLink',
            dataIndex: 'keyLink',
            key: 'keyLink',
            width: 300
        },*/
        {
            title: '№ свой',
            dataIndex: 'selfNumber',
            key: 'selfNumber',
            width: 100,
            showSorterTooltip: false,
            sorter: (a, b) => {
                const f = a.selfNumber - b.selfNumber;
                console.log(f);
                return f
            },
            sortDirections: sortDirections,
            defaultSortOrder: defaultSortOrder,
            sortOrder: options.sortedInfo.columnKey === 'selfNumber' && options.sortedInfo.order
        },
        {
            title: '№ чужой',
            dataIndex: 'alienNumber',
            key: 'alienNumber',
            width: 100,
            //sorter: (a, b) => a.alienNumber - b.alienNumber,
            showSorterTooltip: false,
            //sorter: (a, b) => a.alienNumber >= b.alienNumber ? 1 : -1,
            sorter: (a, b) => a.alienNumber - b.alienNumber,
            sortDirections: sortDirections,
            defaultSortOrder: defaultSortOrder,
            sortOrder: options.sortedInfo.columnKey === 'alienNumber' && options.sortedInfo.order,
        },
        {
            title: 'Время создания',
            dataIndex: 'createDate',
            key: 'createDate',
            width: 150,
            showSorterTooltip: false,
            sorter: (a, b) => a.createDate - b.createDate,
            sortDirections: sortDirections,
            defaultSortOrder: defaultSortOrder,
            sortOrder: options.sortedInfo.columnKey === 'createDate' && options.sortedInfo.order
        },
        {
            title: 'Время закрытия',
            dataIndex: 'destroyDate',
            key: 'destroyDate',
            width: 150
        },
        {
            title: 'Состояние заявки',
            dataIndex: 'userState',
            key: 'userState',
            width: 150,
            showSorterTooltip: false,
            sorter: (a, b) => a.userState.localeCompare(b.userState),
            sortDirections: sortDirections,
            defaultSortOrder: defaultSortOrder,
            sortOrder: options.sortedInfo.columnKey === 'userState' && options.sortedInfo.order,

            filters: [...new Set(data.map(obj => obj.userState))].
                sort((a, b) => a.localeCompare(b)).map(item => ({ text: item, value: item })),

            onFilter: (value, record) => record.userState === value

            /*         
        !! это подробная запись из filters !!
        const ss = new Set();
        data.forEach(item => ss.add(item.state));
        const res = [...ss].sort((a, b) => a.localeCompare(b)).map(item => ({ text: item, value: item }));
        console.log(ss);
        console.log(res);
        */

        },
        {
            title: 'Состояние работ',
            dataIndex: 'workState',
            key: 'workState',
            width: 150
        },
        {
            title: 'Категория',
            dataIndex: 'category',
            key: 'category',
            width: 150
        },
        {
            title: 'Вид ремонта',
            dataIndex: 'repairType',
            key: 'repairType',
            width: 150
        },
        {
            title: 'Энергообъект',
            dataIndex: 'containerName',
            key: 'containerName',
            width: 150,
            showSorterTooltip: false,
            sorter: (a, b) => a.containerName.localeCompare(b.containerName),
            sortDirections: sortDirections,
            defaultSortOrder: defaultSortOrder,
            sortOrder: options.sortedInfo.columnKey === 'containerName' && options.sortedInfo.order,

            filters: [...new Set(data.map(obj => obj.containerName))].
                sort((a, b) => a.localeCompare(b)).map(item => ({ text: item, value: item })),

            onFilter: (value, record) => record.containerName === value

        },
        {
            title: 'Оборудование',
            dataIndex: 'equipmentName',
            key: 'equipmentName',
            width: 150,

            sorter: (a, b) => a.equipmentName.localeCompare(b.equipmentName),
            sortDirections: sortDirections,
            defaultSortOrder: defaultSortOrder,
            sortOrder: options.sortedInfo.columnKey === 'equipmentName' && options.sortedInfo.order,

            filters: [...new Set(data.map(obj => obj.containerName))].
                sort((a, b) => a.localeCompare(b)).
                map(item => ({
                    text: item, value: item, children: [...new Set(data.filter(item2 => item2.containerName === item).map(obj => obj.equipmentName))].
                        sort((a, b) => a.localeCompare(b)).
                        map(item3 => ({ text: item3, value: [item3, item] })) // 0 = equipmentName, 1 = containerName
                })),
            filterMode: 'tree',
            onFilter: (value, record) => {
                if (Array.isArray(value)) {
                    console.log('value = ' + value[0] + ' ' + value[1] + ' ' + value.length + ' record = ' + record.keyLink)
                    const f = Array.isArray(value) && record.containerName === value[1] && record.equipmentName === value[0];
                    console.log(f);
                    return f;
                }
            }
        },
        {
            title: 'Состояние оборудования',
            dataIndex: 'deviceState',
            key: 'deviceState',
            width: 150
        },
        {
            title: 'Просимое время',
            dataIndex: 'needRepairDateBegin',
            key: 'needRepairDateBegin',
            width: 150
        },
        {
            title: 'Разрешенное время',
            dataIndex: 'permitRepairDateBegin',
            key: 'permitRepairDateBegin',
            width: 150
        },
        {
            title: 'Фактическое время',
            dataIndex: 'factRepairDateBegin',
            key: 'factRepairDateBegin',
            width: 150
        },
        {
            title: 'Плановое время',
            dataIndex: 'planDateBegin',
            key: 'planDateBegin',
            width: 150
        },
    ];


    const onChange = (pagination, filters, sorter) => {
        console.log('onChange', pagination, filters, sorter);
        //console.log('filter = ' + filters);
        //console.log('sorter = ' + sorter);
        setOptions({ ...options, filteredInfo: filters, sortedInfo: sorter })
    };


    console.log('return');

    return (

        < Table
            bordered size='small' pagination={false} scroll={{ y: '100%', x: '2000px' }
            }
            columns={columns} dataSource={data} rowKey='keyLink' onChange={onChange} >
        </Table >
    )
}