import React from 'react';
import './Toolbar.css';
import logo from '../../../public/logo.png'


const Toolbar = props => (
        <header className="toolbar">
            <nav className="toolbar_navigation">
                <div className="toolbar_toggle-button"></div>
                <div className="toolbar_logo" onClick={()=> props.changeType("homepage")} style={{cursor: 'pointer'}}>
                    <a ><img src={logo} alt="C"/></a>
                    <h1>Scout</h1>
                </div>
                <div className="spacer"></div>
                <div className="toolbar_navigation-items">
                    <ul>
                        <li onClick={()=>(props.changeType("files"))} style={{cursor: 'pointer'}}><a >Files</a></li>
                        <li onClick={()=>(props.changeType("identifiers"))} style={{cursor: 'pointer'}}><a >Identifiers</a></li>
                        <li onClick={()=>(props.changeType("funcs"))} style={{cursor: 'pointer'}}><a style={{top: '8px',bottom: '8px'}}>Functions<br/>and Macros</a></li>
                        <li onClick={()=>(props.changeType("operations"))} style={{cursor: 'pointer'}}><a>Operations</a></li>
                    </ul>
                </div>
            </nav>
        </header>
    );


export default Toolbar;