import React,{Component} from 'react';
import Tabs from '../../Tabs/Tabs';
import Details from './Details';
import SourceControl from '../Source/SourceControl';
import Table from '../../Table';
import axios from 'axios';
import './FilePage';

class Files extends Component {
    constructor(props) {
        super(props);
        this.state={
            loaded:false,
            file:null
        }
        
    };

    componentDidMount(){
        this.getFileInfo();
    }

    getFileInfo = () => {
        axios.get(global.address + "file.html?id="+this.props.id)
        .then((response) => {
            this.setState({
                file: response.data,
                loaded:true
            })
        
        })
    }
 
    
    render(){
        if (this.state.loaded===false)
        return(
            <div>
                <h2>
                    Loading...
                </h2>
            </div>
        );
    else{
        var tabs = {}
        if(this.state.file!==null)                
            tabs = [ 
                {
                    title:"Details",
                    content: <Details dets={this.state.file}/>
                },
                {
                    title:"Metrics",
                    content: <Table head={["Metrics","Values"]} contents={this.state.file.metrics}/>
                },
                {
                    title:"Source",
                    content: <SourceControl id={this.state.file.queries.id} changeType={this.props.changeType}/>
                }
               
        ];
        
        return(
            <div className="FileInfo">
            {(this.state.file===null)?<p>No file selected</p>
            :<div>
                <h2>
                    {this.state.file.pathname}
                </h2>                      
            
            <Tabs children={tabs}/>
            </div>
            }
            </div>
        );
        }
    }
}
export default Files;