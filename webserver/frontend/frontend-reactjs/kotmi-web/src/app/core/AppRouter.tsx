import { Route, Switch } from 'react-router-dom';
import Retrospective from '../components/Retrospective';
import AuthPage from './AuthPage';
import About from './About';

export default function ContentRouter() {
    return (
        <Switch>
            <Route exact path="/" component={About} />
            <Route exact path="/retrospective/:id" component={Retrospective} />
            <Route exact path="/authentication" component={AuthPage} />
        </Switch>
    );
}
