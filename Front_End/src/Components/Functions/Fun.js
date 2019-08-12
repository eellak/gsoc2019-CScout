import React, {Component} from 'react';
import axios from 'axios';
import Table from '../Table';
import Tabs from '../Tabs/Tabs';
import FunList from './FunList';
import './Fun.css';
import ReactSVG from 'react-svg';
import GraphControl from './GraphSelector';


class Fun extends Component {
    constructor(props) {
        super(props);
        this.state = {
            loaded: false,
            fun: null
        }
        this.returnHtml = this.returnHtml.bind(this);
        this.optionsState = 'B';
        this.handleSubmit = this.handleSubmit.bind(this);
        this.handleInputChange = this.handleInputChange.bind(this);
        this.inputValue = '';
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


    handleSubmit(e) {
        e.preventDefault();
        axios.get(global.address + "fun.html?f=" + this.props.f)
        .then((resp) => {
            this.setState({refactor: resp.data})
            console.log(resp)
        }
        )
        
    }

    handleInputChange(e) {
        this.inputValue = e.target.value
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
                        Associated identifiers:
                            {this.state.fun.data.map((obj,i)=>
                                <div key={i} style={{display:"inline"}}>
                                    {obj.identifiers.map((id,j) =>
                                         <a onClick={() => this.props.changeType("id", id.id)} key={j} style={{cursor:"pointer"}}>{id.name}</a> 
                                    )}
                                </div>  
                                )}
                            { this.state.fun.declared?
                            <div>
                                {"Declared in "}
                                <a onClick={() => {
                                    this.props.changeType("filePage",this.state.fun.declared.tokid);
                                    }}
                                    style={{cursor:"pointer"}} >
                                {this.state.fun.declared.tokpath}
                                </a>
                               
                                <a onClick={() => {
                                    this.props.setCall(this.props.f);
                                    this.props.changeType("qsource",this.state.fun.declared.tokid);
                                    }}
                                    style={{cursor:"pointer"}} >
                                    {" Source"}
                                </a>
                             </div>
                             : <div> Not declarations found </div>
                            }
                            { this.state.fun.defined?
                             <div>
                                {"Defined in File "}
                                <a onClick={() => {
                                    this.props.changeType("filePage",this.state.fun.defined.tokid);
                                    }}
                                    style={{cursor:"pointer"}} >
                                {this.state.fun.defined.tokpath}
                                </a>
                                <a onClick={() => {
                                    this.props.setCall(this.props.f);
                                    this.props.changeType("qsource",this.state.fun.defined.tokid);
                                }}
                                style={{cursor:"pointer"}} >
                                {" Source"}</a>
                            </div> 
                            :<div>
                                No definitions found
                            </div>
                            } 
                            <div>
                                {"This function calls directly " + this.state.fun.no_call + " functions and is called by " + this.state.fun.no_called + " functions."}
                            </div>
                            <div>
                            <form onSubmit={this.handleSubmit}>
                                <div className='textSearch'>
                                    Refactor:<input type='text' value={this.state.value} onChange={this.handleInputChange}
                                    placeholder="Refactor Arguments..." /><br />
                                </div>
                            </form>
                               </div>
                        </div>
                    },
                    {
                        title: "Metrics",
                        content: <Table head={["Metrics", "Values"]} contents={!this.state.fun.metrics?[]:this.state.fun.metrics.data} />
                    },
                    {
                        title: "Called by Functions",
                        content: <FunList f={this.props.f} n={"u"} className="lists" changeType={this.props.changeType}/>
                    },
                    {
                        title: "Calls Functions",
                        content: <FunList f={this.props.f} n={"d"} className="lists" changeType={this.props.changeType}/>
                    },
                    {   title: "Graph",
                        content: <GraphControl f={this.props.f} changeType={this.props.changeType}/>
                    }
                ];

            return (
                <div className="FileInfo">
                    {(this.state.fun === null) ? <p>No file selected</p>
                        : <div>
                            <h2>
                                Function:{this.state.fun.fname}
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