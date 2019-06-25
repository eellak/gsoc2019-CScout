import React,{ Component } from 'react';
import './Toolbar.css';
import logo from '../../public/logo.png'


const Toolbar = props => (
        <header className="toolbar">
            <nav className="toolbar_navigation">
                <div className="toolbar_toggle-button"></div>
                <div className="toolbar_logo"><a onClick={()=> props.changeType("homepage")}><img src={logo} alt="Cscout"/></a></div>
                <div className="spacer"></div>
                <div className="toolbar_navigation-items">
                    <ul>
                        <li><a onClick={()=>(props.changeType("files"))} style={{cursor: 'pointer'}}>Files</a></li>
                        <li><a onClick={()=>(props.changeType("identifiers"))} style={{cursor: 'pointer'}}>Identifiers</a></li>
                        <li><a onClick={()=>(props.changeType("funcs"))} style={{cursor: 'pointer'}}>Functions and Macros</a></li>
                        <li><a onClick={()=>(props.changeType("operations"))} style={{cursor: 'pointer'}}>Operations</a></li>
                    </ul>
                </div>
            </nav>
        </header>
    );


export default Toolbar;