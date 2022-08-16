import React from 'react';

import { Table } from 'antd';

const data = [
    {
        keyLink: '1',
        name: 'Mike',
        age: 32,
        address: '10 Downing Street',
    }/*,
    {
        keyLink: '2',
        name: 'John',
        age: 42,
        address: '10 Downing Street',
    },
    {
        keyLink: '3',
        name: 'John',
        age: 42,
        address: '10 Downing Street',
    },
    {
        keyLink: '4',
        name: 'John',
        age: 42,
        address: '10 Downing Street',
    },
    {
        keyLink: '5',
        name: 'John',
        age: 42,
        address: '10 Downing Street',
    },
    {
        keyLink: '6',
        name: 'John',
        age: 42,
        address: '10 Downing Street',
    },
    {
        keyLink: '7',
        name: 'John',
        age: 42,
        address: '10 Downing Street',
    },
    {
        keyLink: '8',
        name: 'John',
        age: 42,
        address: '10 Downing Street',
    },
    {
        keyLink: '9',
        name: 'John',
        age: 42,
        address: '10 Downing Street',
    },
    {
        keyLink: '10',
        name: 'John',
        age: 42,
        address: '10 Downing Street',
    },
    {
        keyLink: '11',
        name: 'John 11',
        age: 42,
        address: '10 Downing Street',
    },
    {
        keyLink: '12',
        name: 'John',
        age: 42,
        address: '10 Downing Street',
    },
    {
        keyLink: '13',
        name: 'John',
        age: 42,
        address: '10 Downing Street',
    },
    {
        keyLink: '14',
        name: 'John 14',
        age: 42,
        address: '10 Downing Street',
    },*/
];

const columns = [
    {
        title: 'Name',
        dataIndex: 'name',
        key: 'name',
    },
    {
        title: 'Age',
        dataIndex: 'age',
        key: 'age',
    },
    {
        title: 'Address',
        dataIndex: 'address',
        key: 'address',
    },
];

export const TestModule = () => {
    //const data = [];

    return (
        <div className='tabcontent'>
            <div><h1>123</h1></div>
            <Table
                bordered size='small' pagination={false} scroll={{ y: '100%', x: '2000px' }}
                columns={columns} dataSource={data} rowKey='keyLink'>
            </Table>
        </div>
    )
}

/*
<div className='tabcontent'>
            <div><h1>123</h1></div>
            <Table
                bordered size='small' pagination={false} scroll={{ y: '100%' }}
                columns={columns} dataSource={data} rowKey='keyLink'>
            </Table>
        </div>
*/

/* <Table
            bordered size='small' pagination={false} scroll={{ y: '100%', x: '2000px' }}
            columns={columns} dataSource={data} rowKey='keyLink'>
        </Table> */

/*
<div className='tabcontent'>
    <div className='toolbar'>
    </div>
    <div style={{ backgroundColor: 'red' }}>
        <Table
            bordered size='small' pagination={false} scroll={{ y: '100%', x: '2000px' }}
            columns={columns} dataSource={data} rowKey='keyLink'>
        </Table>
    </div>
</div> */
/*
<div>
    <div><h1>123</h1></div>
    <Table
        bordered size='small' pagination={false} scroll={{ y: '100%', x: '2000px' }}
        columns={columns} dataSource={data} rowKey='keyLink'>
    </Table>
</div> */