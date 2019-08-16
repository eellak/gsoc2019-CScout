import React from 'react';
import './Toolbar.css';
import logo from '../../../public/logo.png';
import DrawerToggleButton from './DrawerToggleButton';
import Popup from 'reactjs-popup';
import Refactorings from '../Refactorings';

const Toolbar = props => (
    <header className="toolbar">
        <nav className="toolbar_navigation">
            <div className="toolbar_toggle-button"><DrawerToggleButton click={props.drawerClickHandler} /></div>
            <div className="toolbar_logo" onClick={() => props.changeType("homepage")} style={{ cursor: 'pointer' }}>
                <a ><img src={logo} alt="C" /></a>
                <h1>Scout</h1>
            </div>
            <div className="spacer"></div>
            <div className="toolbar_navigation-items">
                <ul>

                    <li>
                        <Popup
                        trigger={<a>Function refactorings</a>} modal closeOnDocumentClick closeOnEscape>
                            <Refactorings changeType={props.changeType}/>
                        </Popup>
                    </li>

                    <li onClick={() => (props.changeType("files"))} style={{ cursor: 'pointer' }}><a >Select project</a></li>
                    <li onClick={() => (props.changeType("identifiers"))} style={{ cursor: 'pointer' }}><a >Identifiers</a></li>
                    <li onClick={() => (props.changeType("funcs"))} style={{ cursor: 'pointer' }}><a style={{ top: '8px', bottom: '8px' }}>Functions<br />and Macros</a></li>
                    <li onClick={() => (props.changeType("graph"))} style={{ cursor: 'pointer' }}><a>Operations</a></li>
                </ul>
            </div>
        </nav>
    </header>
);

export default Toolbar;