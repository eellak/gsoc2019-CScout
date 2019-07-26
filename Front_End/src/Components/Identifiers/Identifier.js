import React, { Component } from 'react';
import Axios from 'axios';
import '../../global.js';
import IdDependancies from './IdDependancies';

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
        var values = ["Ordinary identifier", "Macro", "Undefined macro", "File scope", "Yacc identifier", "Crosses file boundary"]
        return (
            <div>
                <h3>
                    Details
                </h3>
                <table style={{
                    textAlign: 'left'
                }}>

                    <tbody>
                        {
                            values.map((obj, i) =>
                                <tr key={i}>
                                    <td>
                                        {obj.names}
                                    </td>
                                    <td>
                                        {obj}
                                    </td>
                                </tr>
                            )
                        }
                    </tbody>
                </table>
            </div>
        )
    }

    render() {
        return (
            <div>
                {this.state.loaded ?
                    <div>{
                        this.getDetails()
                    }
                        <a>This identifier was matched {this.state.data.occurences} occurences<br />
                            This identifier appears in the following projects:
                    </a>
                        <ul>
                            {this.state.data.projects.content.map((obj, i) =>
                                <li style={{ listStyle: 'none' }} key={i}>{obj}</li>
                            )}
                        </ul>
                        <IdDependancies search={"ec=" + this.state.data.ec + "&qf=1&n=Dep+F+for+ID"} changeType={this.props.changeType} />
                        <IdDependancies search={"ec=" + this.state.data.ec + "&qi=1&n=Dep+F+for+ID"} changeType={this.props.changeType} />

                    </div> : <div>Loading ... </div>}
            </div>
        )
    }
}
export default Identifier;