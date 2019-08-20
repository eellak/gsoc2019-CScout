import React, { Component } from 'react';
import './Main.css';

class Main extends Component {
    constructor() {
        super();
        this.state = {

        }
    }

    render() {
        return (
            <div className='mainpage'>
                <div>
                    <h2 onClick={() => this.props.changeType("files")} className='link'>Files</h2>
                    <p onClick={() => this.props.changeType("browse")}
                        className='link'>Browse Files</p>
                    <p onClick={() => this.props.changeType("searchf")}
                        className='link'>Search File</p>
                    <p onClick={() => this.props.changeType("filemetrics")}
                        className='link'>File Metrics</p>

                </div>

                <div>
                    <h2 className='link'>Identifers</h2>
                    <p onClick={() => this.props.changeType("searchId")}
                        className='link'>Search Identifier</p>
                    <p onClick={() => this.props.changeType("idmetrics")}
                        className='link'>Identifier Metrics</p>
                </div>

                <div>
                    <h2 className='link'> Functions and Macros</h2>
                    <p onClick={() => this.props.changeType("searchfun")}
                        className='link'>Search Functions</p>
                    <p onClick={() => this.props.changeType("funmetrics")}
                        className='link'>Function Metrics</p>
                </div>            
            </div>
        )
    }
}
export default Main;