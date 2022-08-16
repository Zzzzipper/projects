import React, { useState, useEffect } from 'react';
import './styles/App.less';

import WorkLayout from './layouts/WorkLayout';
import LoginLayout from './layouts/LoginLayout';
import { checkStatus } from './common/Api';

import { ConfigProvider } from 'antd';
import ruRU from 'antd/lib/locale/ru_RU';

function App() {

    const [isLoggedIn, setIsLoggedIn] = useState(false);

    useEffect(() => {
        checkStatus()
            .then(result => {
                console.log('code = ' + result.code);
                setIsLoggedIn(result.code >= 0);
            });
    }, []);

    const onLoggedIn = () => {
        setIsLoggedIn(true);
        console.log('onLoggedIn');
    }

    const onLoggedOut = () => {
        setIsLoggedIn(false);
        console.log('onLoggedOut');
    }

    const ActiveLayout = ({ f }) => {
        return f ? <WorkLayout onLoggedOut={onLoggedOut} /> : <LoginLayout onLoggedIn={onLoggedIn} />;
    }

    return (
        <ConfigProvider locale={ruRU}>
            <ActiveLayout f={isLoggedIn} />
        </ConfigProvider>
    );


    /*
        return (
            <WorkLayout></WorkLayout>
        )
    */
}

export default App;
