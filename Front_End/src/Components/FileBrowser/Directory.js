import React, {Component} from 'react';
import axios from 'axios';
import '../../global.js';
import folder from './folder.svg';
import openFolder from './openFolder.svg';
import file from './file.ico';

class Directory extends Component {
    constructor(props) {
        super(props);
        console.log(props);
        this.state = {
            loaded:false
        }
      
    };

    componentDidMount(){
        this.getDir();
    }

    changeExpand = () => {
        this.setState({
            expand: !this.state.expand
        });
    }

    getDir = () => {
        console.log(global.address + this.props.addr);
        axios.get(global.address + this.props.addr)
        .then((response) => {
            if(response.data.error){
                this.state = {
                    error: response.data.error,
                    loaded: false
                }
            } else{
                console.log(response.data);
                this.setState ({
                    loaded:true,
                    tree: response.data.tree,
                    dir: response.data.dir,
                    name: this.props.name,
                    expand: this.props.expand
                });     
            }     
        });
    }

    render(){
        return(
            <div>
                {
                    this.state.loaded
                    ? <ul className='project' style={{listStyleType:"none", listStylePosition:"inside"}}>
                        <li onClick={this.changeExpand} style={{cursor:'pointer', margin:'10px', textAlign:'left'}}>
                            <img src={this.state.expand?openFolder:folder}/> {this.state.name}</li> 
                        {this.state.expand
                            ?  <li>
                            {this.state.tree.children.map((child,i) => 
                                <div key={i}>
                                {
                                    (child.info.type==="dir")
                                    ?<Directory addr={"dir.html?dir=" + child.addr} expand={false} 
                                    onClick={console.log(child)} name={child.info.name}/>             
                                    :<ul style={{listStyleType:"none", listStylePosition:"inside"}}>
                                        <li style={{cursor:'pointer', margin:'10px', textAlign:'left'}}><img src={file} style={{width:'14px', height:'16px'}}/>  {child.info.name}</li>
                                    </ul>
                                }
                                </div>
                            )}</li>
                            :<li/>
                        } 
                    </ul>
                    :<div>Loading...</div>
                }
            </div>
        )
    }

    
}

export default Directory;