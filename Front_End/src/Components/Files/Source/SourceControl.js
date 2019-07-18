import React,{Component} from 'react';
import Source from './Source';
import './SourceControl.css';

class SourceControl extends Component{
    constructor(props){
        super(props);
        this.state = {
            optionsState:'0'
        }
    }

    change(event){
        console.log(event);
        this.setState({
            optionsState: event.target.value
        })
        event.preventDefault();
    }

    render(){
        return(
            <div className="selector" >
                <select value={this.state.optionsState} onChange={this.change.bind(this)}>
                    <option value='0' className="selected">Plain text</option>
                    <option value='1'>Unprocessed regions marked</option>
                    <option value='2'>Identifier hyperlinks</option>
                    <option value='3'>Project-global writable identifiers links</option>
                    <option value='4'>Function and macro links</option>
                </select>
                <Source id={this.props.id} type={this.state.optionsState} changeType={this.props.changeType}/>
            </div>
        )
    }
}

export default SourceControl;