import React, { Component } from 'react';
import './Details.css';

class Details extends Component {
    constructor(props) {
        super(props);
        this.state = {

        }
    }

    render() {
        return (
            <div className="details">
                <div>Read-Only: {this.props.dets.readonly ? "Yes" : "No"}</div>
                {
                    this.props.dets.hasOwnProperty("files") ?
                        <div>Used in Following Project(s)
                        <ul>
                                {this.props.dets.files.map((proj, i) =>
                                    <li key={i}>{proj}</li>
                                )}
                            </ul>
                        </div>
                        : <div>Not used in projects</div>
                }
                {
                    this.props.dets.hasOwnProperty("copies") ?
                        <div>Other exact copies of this files
                        <ul>
                                {this.props.dets.copeis.map((path, i) =>
                                    <li key={i}>{path}</li>
                                )}
                            </ul>
                        </div>
                        : <div>There are no other exact copies of this file</div>
                }
            </div>
        )
    }
}
export default Details;