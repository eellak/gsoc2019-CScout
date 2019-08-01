import React, { Component } from 'react';
import Axios from 'axios';
import '../../global.js';
import IdDependancies from './IdDependancies';
import Check from './check.ico';
import X from '../x.ico';
import './Identifier.css';
import Tabs from '../Tabs/Tabs';
class Identifier extends Component {
    constructor(props) {
        super(props);
        this.state = {
            loaded: false
        }

    }

    componentDidMount() {
        this.getIdentifier();
    }

    getIdentifier() {
        Axios.get(global.address + 'id.html?id=' + this.props.id)
            .then((response) => {
                this.setState({
                    loaded: true,
                    data: response.data
                })
            })
    }

    getDetails() {
        console.log(this.state.data.attribute)
        return (
                <table >
                    <tbody>
                        {
                            this.state.data.attribute.map((obj, i) =>
                                <tr key={i}>
                                    <td>
                                        {obj.name}
                                    </td>
                                    <td>
                                    <img src={obj.get?Check:X} alt="x" height="19" width="19"/>
                                    </td>
                                </tr>
                            )
                        }
                        <tr>
                            <td>{this.state.data.file_boundary.name}</td>
                            <td><img src={this.state.data.file_boundary.get?Check:X} alt="x" height="19" width="19"/></td>
                        </tr>
                    </tbody>
                </table>
        )
    }

    render() {
        var tabs = {}
        if (this.state.loaded)
            tabs = [
                {
                    title: "Details",
                    content: 
                    <div className="IdDetails">        
                    {
                        this.getDetails()
                    }
                    <div className="textIdDetails">
                        <a>This identifier was matched {this.state.data.occurences} occurences<br />
                            This identifier appears in the following projects:
                        </a>
                        <ul>
                            {this.state.data.projects.content.map((obj, i) =>
                                <li style={{ listStyle: 'none' }} key={i}>{obj}</li>
                            )}
                        </ul>
                    </div>
                </div>
                },
                {
                    title: "Dependant Files",
                    content: <IdDependancies search={"xiquery.html?ec=" + this.state.data.ec + "&qf=1&n=Dep+F+for+ID"} changeType={this.props.changeType} />
                },
                {
                    title: "Associated Functions",
                    content: <IdDependancies search={"xfunquery.html?ec=" + this.state.data.ec + "&qi=1&n=Dep+F+for+ID"} changeType={this.props.changeType} />
                }
            ];
        
        
        return (
            <div>
                {this.state.loaded ?
                    <div>
                        <h2>Identifier Page:{this.state.data.identifier}</h2>
                
                        <Tabs children={tabs} />

                    </div> : <div>Loading ... </div>}
            </div>
        )
    }
}
export default Identifier;