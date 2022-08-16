import React from 'react';

import 'antd/dist/antd.less';
import '../styles/App.less';


import { Layout, Menu, Dropdown, Button, Tabs, DatePicker, Table, Tooltip } from 'antd';
const { Header, Footer, Content } = Layout;
const { TabPane } = Tabs;
import { DownOutlined, UserOutlined } from '@ant-design/icons';

//import ReloadButton from '../icons/034 reload.png';

function Work() {

    const menu = (
        <Menu onClick={handleMenuClick}>
            <Menu.Item key="1" icon={<UserOutlined />}>
                1st menu item
            </Menu.Item>
            <Menu.Item key="2" icon={<UserOutlined />}>
                2nd menu item
            </Menu.Item>
            <Menu.Item key="3" icon={<UserOutlined />}>
                3rd menu item
            </Menu.Item>
        </Menu>
    );

    const columns = [
        {
            title: '№ свой',
            dataIndex: 'selfNumber',
            key: 'selfNumber'
        },
        {
            title: '№ чужой',
            dataIndex: 'alienNumber',
            key: 'alienNumber',
        },
        {
            title: 'Время создания',
            dataIndex: 'createDate',
            key: 'createDate',
        },
        {
            title: 'Время закрытия',
            dataIndex: 'destroyDate',
            key: 'destroyDate',
        },
        {
            title: 'Состояние заявки',
            dataIndex: 'userState',
            key: 'userState',
        },
        {
            title: 'Состояние работ',
            dataIndex: 'workState',
            key: 'workState',
        },
        {
            title: 'Категория',
            dataIndex: 'category',
            key: 'category',
        },
        {
            title: 'Вид ремонта',
            dataIndex: 'repairType',
            key: 'repairType',
        },
        {
            title: 'Энергообъект',
            dataIndex: 'object',
            key: 'object',
        },
        {
            title: 'Оборудование',
            dataIndex: 'equipment',
            key: 'equipment',
        },
        {
            title: 'Состояние оборудования',
            dataIndex: 'deviceState',
            key: 'deviceState',
        },
        {
            title: 'Просимое время',
            dataIndex: 'needRepairDateBegin',
            key: 'needRepairDateBegin',
        },
        {
            title: 'Разрешенное время',
            dataIndex: 'permitRepairDateBegin',
            key: 'permitRepairDateBegin',
        },
        {
            title: 'Фактическое время',
            dataIndex: 'factRepairDateBegin',
            key: 'factRepairDateBegin',
        },
        {
            title: 'Плановое время',
            dataIndex: 'planRepairDateBegin',
            key: 'planRepairDateBegin',
        },
    ];

    const data = [];

    function handleMenuClick(e) {
        // message.info('Click on menu item.');
        console.log('click', e);
    }


    return (
        <Layout className='layout' style={{ height: '100vh' }}>
            <Header style={{ backgroundColor: 'inherit', padding: 5, lineHeight: 0, height: '42px' }}>
                <Button>
                    Menu 1
                </Button>
                <Dropdown overlay={menu} trigger={['click']}>
                    <Button>
                        Menu 2<DownOutlined />
                    </Button>
                </Dropdown>
            </Header>
            <Content style={{ padding: 5, backgroundColor: 'darkgray' }}>
                <Tabs type='editable-card' hideAdd={true} size='small' tabBarStyle={{ marginBottom: 0 }}
                    style={{ height: '100%' }}>
                    <TabPane tab='Заявки АСУРЭО' key='1'>
                        <div className='toolbar'>
                            <span>Время:</span>
                            <DatePicker showTime allowClear='false'></DatePicker>
                            <Button type='link' style={{
                                backgroundColor: 'rgba(64, 169, 255, 0.3)',
                                borderColor: 'rgba(64, 169, 255, 0.07)'
                            }}>
                                <img src={'./icons/034 reload.png'} />
                            </Button>
                            <Tooltip placement='bottomLeft' title='Автоматическое обновление'
                                color='rgb(255,254,189)' overlayInnerStyle={{ color: 'black' }}>
                                <Button type='link'>
                                    <img src={'./icons/033 currenttime.png'} />
                                </Button>
                            </Tooltip>
                            &#124;
                            <Button ghost>
                                <img src={'./icons/151 zone.png'} />
                                Зона ведения
                            </Button>
                            <Button ghost>
                                <img src={'./icons/151 zone.png'} />
                                <span> Зона управления </span>
                            </Button>
                            &#124;
                            <Button ghost>
                                <img src={'./icons/212 report.png'} />
                            </Button>
                            &#124;
                            <span>Всего заявок в зоне ответственности: </span>
                            <Button ghost>
                                <img src={'./icons/052 error.png'} />
                            </Button>
                            <Button ghost>
                                <img src={'./icons/000 info.png'} />
                            </Button>
                        </div>
                        <Table className='components-table-demo-nested'
                            bordered scroll={{ x: 2500 }} size='small'
                            columns={columns} dataSource={data}>
                        </Table>
                    </TabPane>
                    <TabPane tab='Таб 2' key='2'>
                        {'bbb'}
                    </TabPane>
                </Tabs>
            </Content>
            <Footer style={{ padding: 5, textAlign: 'right' }}>
                Имя сервера Фамилия И.О. Дата время
            </Footer>
        </Layout >
    )
}

export default Work;
