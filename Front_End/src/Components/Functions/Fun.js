import React, {Component} from 'react';
import axios from 'axios';
import Table from '../Table';
import Tabs from '../Tabs/Tabs';
import FunList from './FunList';
import './Fun.css'

class Fun extends Component {
    constructor(props) {
        super(props);
        this.state = {
            loaded: false,
            fun: null
        }
        this.returnHtml = this.returnHtml.bind(this);
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

    returnHtml(html){
        return(
            {
                __html: html
            }
        )
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
            console.log(this.state.fun)
            if (this.state.fun !== null)
                tabs = [
                    {
                        title: "Details",
                        content:
                        <div className="funDet"> 
                            { this.state.fun.declared?
                            <div>
                                {"Declared in "}
                                <a onClick={() => 
                                    this.props.changeType("filePage",this.state.fun.declared.tokid)}
                                    style={{cursor:"pointer"}} >
                                    {this.state.fun.declared.tokpath}
                                </a>
                             </div>
                             : <div> Not declared </div>
                            }
                            { this.state.defined?
                             <div>
                                Defined in File
                                {this.state.fun.defined.tokpath}
                            </div> 
                            :<div>
                                Not defined
                            </div>
                            } 
                        </div>
                    },
                    {
                        title: "Metrics",
                        content: <Table head={["Metrics", "Values"]} contents={!this.state.fun.metrics?[]:this.state.fun.metrics.data} />
                    },
                    {
                        title: "Called Functions",
                        content: <FunList f={this.props.f} n={"u"} className="lists" changeType={this.props.changeType}/>
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