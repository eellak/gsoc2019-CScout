import React,{Component} from 'react';
import Axios from 'axios';
import '../../../global.js';
import './FileSearch.css'

class FileSearch extends Component{
    constructor(props){
        super(props)
        this.state = {
            loaded: false
        }

    }

    componentDidMount(){
        this.getFiles();
    }

    getFiles(){
        Axios.get(global.address + "xfilequery.html?ro=1&writable=1&match=Y&n=All+Files")
        .then((response) => {
            if(response.data.error){
                this.setState({
                    error: response.data.error
                })
            } else
            {
                this.setState({
                    files: response.data,
                    loaded:true
                })
            }
        });
    }

    render(){

        return(
            <div>
                <h3>
                    File Search
                </h3>
                {this.state.loaded?
                <table className="FileResults">
                    <thead>
                        <tr>
                            <td>
                                Name
                            </td>
                            <td>
                                Path
                            </td>
                            {
                                this.state.files.metric?
                                <td>Metric</td>
                                :null
                            }
                        </tr>
                    </thead>
                    <tbody>
                    {
                        this.state.files.file.map((obj,i) =>
                            <tr>
                                <td>{obj.name.substr(obj.name.lastIndexOf("/")+1)}</td>
                                <td>{obj.name.substring(0,obj.name.lastIndexOf("/"))}</td>
                                {this.state.files.metric?<td>{obj.metric}</td>:null}
                            </tr>
                        )
                    }
                    </tbody>
                </table>
                :<div>Loading..</div>

                }
            </div>
        )
    }
}
export default FileSearch;