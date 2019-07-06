import React,{Component} from 'react';
import axios from 'axios';


class FBrowse extends Component{
    constructor() {
        super();
        this.state={
            loaded:false
        }
        
    };
    componentDidMount() {
        this.getDir();
    }

    getDir = () => {
        if(this.props.type === "top"){
            axios.get("http://localhost:8081/browseTop.html")
            .then((response) => {
                axios.get("http://localhost:8081/" + response.data.addr)
                .then((resp) => {
                    if(resp.data.error){
                        return resp.data.error;
                    }
                    this.setState({
                        loaded:true,
                        tree: resp.data.tree,
                        dir: resp.data.dir
                    });    
                }
                );
            })
        }     
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
                    
                    <ul style={{listStyleType:"none"}}>
                        <li><a href={this.state.tree.parent}>...</a></li>
                       {this.state.tree.children.map((child,i) => 
                           <li key={i}><a href={child.addr}>  {child.name}</a></li>
                        

                        )}
                    </ul>
                </div>
            );
    }
}
export default FBrowse;