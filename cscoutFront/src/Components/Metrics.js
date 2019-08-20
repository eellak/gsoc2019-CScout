import React, { Component } from 'react';
import axios from 'axios';
import Table from './Table';

class Metrics extends Component {
    constructor() {
        super();
        this.state = {
            loaded: false
        }

    };
    componentDidMount() {
        this.getFileMetrics();
    }

    getFileMetrics = () => {
        axios.get("http://localhost:8081/" + this.props.type + "metrics.html")
            .then((response) => {
                if (response.data.errorMsg) {
                    return response.data.errorMsg;
                } else {

                    console.log(response.data);
                    if (this.props.type === "fun")
                        this.setState({
                            loaded: true,
                            head: response.data.head,
                            metrics: response.data.metrics
                        });
                    else
                        this.setState({
                            loaded: true,
                            writable: response.data.writable,
                            "read-only": response.data["read-only"]
                        });

                }
            });
    }

    render() {
        console.log(this.state);
        if (this.state.loaded === false)
            return (
                <div>
                    <h2>
                        Loading...
                    </h2>
                </div>
            );
        else
            return (
                <div>
                    {
                        (this.props.type === "fun") ? <Table head={this.state.head} contents={this.state.metrics} /> :
                            <Table head={this.state.writable.head} contents={this.state.writable.metrics} />

                    }
                </div>
            );
    }
};

export default Metrics;