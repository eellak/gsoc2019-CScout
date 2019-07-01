import React,{Component} from 'react';
import axios from 'axios';
import Table from './Table';

class Filemetrics extends Component{
    constructor() {
        super();
        this.state={
            loaded:false
        }
        
    };
    componentDidMount() {
        this.getFileMetrics();
    }

    getFileMetrics = () => {
        axios.get("http://localhost:8081/filemetrics.html")
        .then((response) => {
            if (response.data.errorMsg) {
               return response.data.errorMsg;
            } else  {
               
                console.log(response.data);
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
        if (this.state.loaded===false)
            return(
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
                    
                    <Table head={this.state.writable.head} contents={this.state.writable.metrics}/>
                }
            </div>
            );
    }
};

export default Filemetrics;