import React, { useState } from 'react';

import 'antd/dist/antd.less';
import { v4 as uuidv4 } from 'uuid';

import { MODULELIST } from '../ModuleList';

import { Layout, Tabs, Button } from 'antd';
const { Header, Footer, Content } = Layout;
const { TabPane } = Tabs;

import { logout } from '../common/Api';

const WorkLayout = ({ onLoggedOut }) => {

    // работа с табами
    const [tabs, setTabs] = useState([]);
    const [activeTabKey, setActiveTabKey] = useState('');

    const addTab = (caption, content) => {
        const newActiveTabKey = uuidv4();
        const newTabs = [...tabs, { caption: caption, content: content, key: newActiveTabKey }];
        setTabs(newTabs);
        setActiveTabKey(newActiveTabKey);
    };

    const removeTab = (targetKey) => {
        let lastIndex = -1;
        tabs.forEach((tab, idx) => {
            if (tab.key === targetKey)
                lastIndex = idx - 1;
        });

        const newTabs = tabs.filter(tab => tab.key !== targetKey);

        let newActiveTabKey = activeTabKey;
        if (newTabs.length && newActiveTabKey === targetKey) {
            if (lastIndex >= 0)
                newActiveTabKey = newTabs[lastIndex].key;
            else
                newActiveTabKey = newTabs[0].key;
        }

        setTabs(newTabs);
        setActiveTabKey(newActiveTabKey);
    };

    const onTabEdit = (targetKey, action) => {
        if (action === 'remove')
            removeTab(targetKey);
    };

    const onTabChange = (activeKey) => {
        setActiveTabKey(activeKey);
    };

    const getModule = (name) => {
        const module = MODULELIST.find(item => item.name === name);
        return module.module;
    }

    // обработчики нажатий на кнопки меню
    const onBtnClick1 = (event) => {
        const module = getModule('AsureoViewer');
        addTab(event.currentTarget.textContent, module);
    };

    const onBtnClick2 = (event) => {
        const module = getModule('TestModule');
        addTab(event.currentTarget.textContent, module);
    };

    const onExitClick = (event) => {
        logout().then(result => {
            console.log(result);
            if (result.code >= 0)
                onLoggedOut();
        });
    };

    return (
        <Layout className='layout' style={{ height: '100vh' }}>
            <Header style={{
                backgroundColor: 'inherit', padding: 5, lineHeight: 0, height: '42px',
                display: 'flex', justifyContent: 'space-between'
            }}>
                <div>
                    <Button onClick={onBtnClick1}>
                        Заявки АСУРЭО
                    </Button>
                    <Button onClick={onBtnClick2}>
                        Test
                    </Button>
                </div>
                <Button onClick={onExitClick}>
                    Выход
                </Button>
            </Header>
            <Content style={{ padding: 5, backgroundColor: 'darkgray' }}>
                <Tabs type='editable-card' hideAdd={true} size='small' tabBarStyle={{ marginBottom: 0 }}
                    activeKey={activeTabKey} onEdit={onTabEdit} onChange={onTabChange} style={{ height: '100%' }}>
                    {tabs.map(tab => (
                        <TabPane tab={tab.caption} key={tab.key}>
                            {tab.content}
                        </TabPane>
                    ))}
                </Tabs>
            </Content>
            <Footer style={{ padding: 5, textAlign: 'right' }}>
                Имя сервера Фамилия И.О. Дата время
            </Footer>
        </Layout>
    )
};

export default WorkLayout;

/*
<TabPane tab='Заявки АСУРЭО' key='1'>
                        <AsureoViewer></AsureoViewer>
                    </TabPane>
                    <TabPane tab='Таб 2' key='2'>
                        {'bbb'}
                    </TabPane>
*/