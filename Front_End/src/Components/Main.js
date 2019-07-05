import React,{Component} from 'react';
import './Main.css';

class Main extends Component {
    constructor(){
        super();
        this.state = {

        }
    }

    render() {
        return(
            <div className='mainpage'>
                <div>
                    <h2 onClick={()=> this.props.changeType("files")} className='link'>Files</h2>
                    <p onClick={()=> this.props.changeType("browse")} 
                    className='link'>Browse Files</p>
                    <p onClick={()=> this.props.changeType("searchf")} 
                    className='link'>Search File</p>
                    <p onClick={()=> this.props.changeType("filemetrics")} 
                    className='link'>File Metrics</p>

                </div>
                
                <div>
                    <h2 onClick={()=> this.props.changeType("identifers")} 
                        className='link'>Identifers</h2>
                
                </div>
                   
                <div>
                    <h2 className='link'> Functions and Macros</h2>
                </div>

                <div>
                    <h2 className='link'> File Dependancies</h2>
                </div>
            </div>
        )
    }
}
export default Main;