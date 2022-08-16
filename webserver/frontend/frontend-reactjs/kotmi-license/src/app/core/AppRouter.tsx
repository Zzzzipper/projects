import { Route, Switch } from 'react-router-dom';
import License from '../components/License';

export default function ContentRouter() {
    return (
        <Switch>
            <Route exact path="/" component={License} />
            <Route exact path="/license" component={License} />
        </Switch>
    );
}
