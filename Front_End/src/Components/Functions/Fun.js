import React, {Component} from 'react';
import axios from 'axios';
import Table from '../Table';
import Tabs from '../Tabs/Tabs';

class Fun extends Component {
    constructor(props) {
        super(props);
        this.state = {
            loaded: false,
            fun: null
        }

    };

    componentDidMount() {
        this.getFunInfo();
    }

    componentDidUpdate(prevProps){
        if (prevProps.f !== this.props.f) {
            this.getFunInfo()
        }
    }

    getFunInfo = () => {
        axios.get(global.address + "fun.html?f=" + this.props.f)
            .then((response) => {
                this.setState({
                    fun: response.data,
                    loaded: true
                })

            })
    }

    render() {
        if (this.state.loaded === false)
            return (
                <div>
                    <h2>
                        Loading...
                </h2>
                </div>
            );
        else {
            var tabs = {}
            console.log(this.state)
            if (this.state.fun !== null)
                tabs = [
                    {
                        title: "Details",
                        content: <div>det</div>
                    },
                    {
                        title: "Metrics",
                        content: <Table head={["Metrics", "Values"]} contents={!this.state.fun.data.metrics?[]:this.state.fun.metrics.data} />
                    },
                    {
                        title: "tes",
                        content: <div>test</div>
                    }
                ];

            return (
                <div className="FileInfo">
                    {(this.state.fun === null) ? <p>No file selected</p>
                        : <div>
                            <h2>
                                {this.state.fun.fname}
                            </h2>

                            <Tabs children={tabs} />
                        </div>
                    }
                </div>
            );
        }
    }
}
export default Fun;